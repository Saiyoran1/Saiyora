#include "BuffHandler.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#pragma region Initialization

UBuffHandler::UBuffHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBuffHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Buff Handler."), *GetOwner()->GetActorLabel());
}

void UBuffHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	return;
}

bool UBuffHandler::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	bWroteSomething |= Channel->ReplicateSubobjectList(Buffs, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(Debuffs, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(HiddenBuffs, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(RecentlyRemoved, *Bunch, *RepFlags);

	return bWroteSomething;
}

#pragma endregion 
#pragma region Application

FBuffApplyEvent UBuffHandler::ApplyBuff(TSubclassOf<UBuff> const BuffClass, AActor* const AppliedBy,
		UObject* const Source, bool const DuplicateOverride, EBuffApplicationOverrideType const StackOverrideType,
		int32 const OverrideStacks, EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration,
		bool const IgnoreRestrictions, TArray<FCombatParameter> const& BuffParams)
{
    FBuffApplyEvent Event;

    if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveBuffs || !IsValid(AppliedBy) || !IsValid(Source) || !IsValid(BuffClass))
    {
        Event.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

    Event.AppliedBy = AppliedBy;
	UPlaneComponent* GeneratorPlane = nullptr;
	UBuffHandler* GeneratorBuff = nullptr;
	if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		GeneratorPlane = ISaiyoraCombatInterface::Execute_GetPlaneComponent(AppliedBy);
		GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(AppliedBy);
	}
	Event.AppliedByPlane = IsValid(GeneratorPlane) ? GeneratorPlane->GetCurrentPlane() : ESaiyoraPlane::None;
    Event.AppliedTo = GetOwner();
	Event.AppliedToPlane = IsValid(PlaneComponent) ? PlaneComponent->GetCurrentPlane() : ESaiyoraPlane::None;
	Event.AppliedXPlane = UPlaneComponent::CheckForXPlane(Event.AppliedByPlane, Event.AppliedToPlane);
    Event.Source = Source;
    Event.BuffClass = BuffClass;
	Event.CombatParams = BuffParams;

    if (!IgnoreRestrictions && (CheckIncomingBuffRestricted(Event) || (IsValid(GeneratorBuff) && GeneratorBuff->CheckOutgoingBuffRestricted(Event))))
    {
    	Event.ActionTaken = EBuffApplyAction::Failed;
    	return Event;
    }

	UBuff* AffectedBuff = FindExistingBuff(BuffClass, AppliedBy);
	if (IsValid(AffectedBuff) && !AffectedBuff->GetDuplicable() && !DuplicateOverride)
	{
		AffectedBuff->ApplyEvent(Event, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
	else
	{
		Event.AffectedBuff = NewObject<UBuff>(GetOwner(), Event.BuffClass);
		Event.AffectedBuff->InitializeBuff(Event, this, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
		switch (Event.AffectedBuff->GetBuffType())
		{
		case EBuffType::Buff :
			Buffs.Add(Event.AffectedBuff);
			break;
		case EBuffType::Debuff :
			Debuffs.Add(Event.AffectedBuff);
			break;
		case EBuffType::HiddenBuff :
			HiddenBuffs.Add(Event.AffectedBuff);
			break;
		default :
			break;
		}
		OnIncomingBuffApplied.Broadcast(Event);
		if (IsValid(GeneratorBuff))
		{
			GeneratorBuff->NotifyOfOutgoingBuffApplication(Event);
		}
	}
    return Event;
}

void UBuffHandler::NotifyOfOutgoingBuffApplication(FBuffApplyEvent const& BuffEvent)
{
	if (BuffEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		OutgoingBuffs.Add(BuffEvent.AffectedBuff);
	}
	OnOutgoingBuffApplied.Broadcast(BuffEvent);
}

#pragma endregion

FBuffRemoveEvent UBuffHandler::RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason)
{
	FBuffRemoveEvent Event;
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Buff) || Buff->GetAppliedTo() != GetOwner())
	{
		Event.Result = false;
		return Event;
	}
	Event.RemovedBuff = Buff;
	Event.AppliedBy = Buff->GetAppliedBy();
	Event.RemovedFrom = GetOwner();
	Event.ExpireReason = ExpireReason;
	EBuffType const BuffType = Buff->GetBuffType();
	TArray<UBuff*>* ArrayToRemoveFrom;
	switch (BuffType)
	{
		case EBuffType::Buff :
			ArrayToRemoveFrom = &Buffs;
			break;
		case EBuffType::Debuff :
			ArrayToRemoveFrom = &Debuffs;
			break;
		case EBuffType::HiddenBuff :
			ArrayToRemoveFrom = &HiddenBuffs;
			break;
		default :
			Event.Result = false;
			return Event;
	}
	Event.Result = ArrayToRemoveFrom->Remove(Buff) > 0;
	if (Event.Result)
	{
		Buff->ExpireBuff(Event);
		OnIncomingBuffRemoved.Broadcast(Event);

		//This is to allow replication one last time (replicating the remove event).
		RecentlyRemoved.Add(Buff);
		FTimerHandle RemoveTimer;
		FTimerDelegate const RemoveDel = FTimerDelegate::CreateUObject(this, &UBuffHandler::PostRemoveCleanup, Buff);
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, RemoveDel, 1.0f, false);

		if (IsValid(Event.AppliedBy) && Event.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(Event.AppliedBy);
			if (IsValid(BuffGenerator))
			{
				BuffGenerator->NotifyOfOutgoingBuffRemoval(Event);
			}
		}
	}
	return Event;
}

void UBuffHandler::PostRemoveCleanup(UBuff* Buff)
{
	RecentlyRemoved.Remove(Buff);	
}

TArray<UBuff*> UBuffHandler::GetBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const
{
	TArray<UBuff*> FoundBuffs;
	if (!BuffClass)
	{
		return FoundBuffs;
	}
	EBuffType const BuffType = BuffClass.GetDefaultObject()->GetBuffType();
	switch (BuffType)
	{
		case EBuffType::Buff :
			for (UBuff* Buff : Buffs)
			{
				if (Buff->GetClass() == BuffClass)
				{
					FoundBuffs.AddUnique(Buff);
				}
			}
			break;
		case EBuffType::Debuff :
			for(UBuff* Buff : Debuffs)
			{
				if (Buff->GetClass() == BuffClass)
				{
					FoundBuffs.AddUnique(Buff);
				}
			}
			break;
		case EBuffType::HiddenBuff :
			for (UBuff* Buff : HiddenBuffs)
			{
				if (Buff->GetClass() == BuffClass)
				{
					FoundBuffs.AddUnique(Buff);
				}
			}
			break;
		default :
			break;
	}
	return FoundBuffs;
}

TArray<UBuff*> UBuffHandler::GetOutgoingBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const
{
	TArray<UBuff*> FoundBuffs;
	if (!BuffClass)
	{
		return FoundBuffs;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (Buff->GetClass() == BuffClass)
		{
			FoundBuffs.AddUnique(Buff);
		}
	}
	return FoundBuffs;
}

UBuff* UBuffHandler::FindExistingBuff(TSubclassOf<UBuff> const BuffClass, AActor* Owner) const
{
	if (!BuffClass)
	{
		return nullptr;
	}
	EBuffType const SearchType = BuffClass.GetDefaultObject()->GetBuffType();
	TArray<UBuff*> const* BuffArray;
	switch (SearchType)
	{
		case EBuffType::Buff :
			BuffArray = &Buffs;
			break;
		case EBuffType::Debuff :
			BuffArray = &Debuffs;
			break;
		case EBuffType::HiddenBuff :
			BuffArray = &HiddenBuffs;
			break;
		default :
			BuffArray = &Buffs;
	}
	for (UBuff* Buff : *BuffArray)
	{
		if (Owner)
		{
			if (Buff->GetClass() == BuffClass && Buff->GetAppliedBy() == Owner)
			{
				return Buff;
			}
		}
		else if (Buff->GetClass() == BuffClass)
		{
			return Buff;
		}
	}
	return nullptr;
}

void UBuffHandler::NotifyOfOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent)
{
	OutgoingBuffs.RemoveSingleSwap(RemoveEvent.RemovedBuff);
	OnOutgoingBuffRemoved.Broadcast(RemoveEvent);
}

void UBuffHandler::NotifyOfReplicatedIncomingBuffApply(FBuffApplyEvent const& ReplicatedEvent)
{
	if (ReplicatedEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		switch (ReplicatedEvent.AffectedBuff->GetBuffType())
		{
		case EBuffType::Buff :
			Buffs.Add(ReplicatedEvent.AffectedBuff);
			break;
		case EBuffType::Debuff :
			Debuffs.Add(ReplicatedEvent.AffectedBuff);
			break;
		case EBuffType::HiddenBuff :
			HiddenBuffs.Add(ReplicatedEvent.AffectedBuff);
			break;
		default :
			break;
		}
	}
	OnIncomingBuffApplied.Broadcast(ReplicatedEvent);
}

void UBuffHandler::NotifyOfReplicatedIncomingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent)
{
	TArray<UBuff*>* BuffArray;
	switch (ReplicatedEvent.RemovedBuff->GetBuffType())
	{
	case EBuffType::Buff :
		BuffArray = &Buffs;
		break;
	case EBuffType::Debuff :
		BuffArray = &Debuffs;
		break;
	case EBuffType::HiddenBuff :
		BuffArray = &HiddenBuffs;
		break;
	default :
		BuffArray = &Buffs;
		break;
	}

	if (BuffArray->RemoveSingleSwap(ReplicatedEvent.RemovedBuff) != 0)
	{
		OnIncomingBuffRemoved.Broadcast(ReplicatedEvent);
	}
}

void UBuffHandler::NotifyOfReplicatedOutgoingBuffApply(FBuffApplyEvent const& ReplicatedEvent)
{
	if (ReplicatedEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		OutgoingBuffs.Add(ReplicatedEvent.AffectedBuff);
	}
	OnOutgoingBuffApplied.Broadcast(ReplicatedEvent);
}

void UBuffHandler::NotifyOfReplicatedOutgoingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent)
{
	if (OutgoingBuffs.RemoveSingleSwap(ReplicatedEvent.RemovedBuff) != 0)
	{
		OnOutgoingBuffRemoved.Broadcast(ReplicatedEvent);
	}
}

#pragma region Subscriptions

void UBuffHandler::SubscribeToIncomingBuff(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingBuffApplied.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromIncomingBuff(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingBuffApplied.Remove(Callback);
	}
}

void UBuffHandler::SubscribeToIncomingBuffRemove(FBuffRemoveCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingBuffRemoved.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromIncomingBuffRemove(FBuffRemoveCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingBuffRemoved.Remove(Callback);
	}
}

void UBuffHandler::SubscribeToOutgoingBuff(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingBuffApplied.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromOutgoingBuff(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingBuffApplied.Remove(Callback);
	}
}

void UBuffHandler::SubscribeToOutgoingBuffRemove(FBuffRemoveCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingBuffRemoved.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromOutgoingBuffRemove(FBuffRemoveCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingBuffRemoved.Remove(Callback);
	}
}

#pragma endregion
#pragma region Restrictions

void UBuffHandler::AddIncomingBuffRestriction(FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingBuffRestrictions.AddUnique(Restriction);
	}
}

void UBuffHandler::RemoveIncomingBuffRestriction(FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingBuffRestrictions.Remove(Restriction);
	}
}

bool UBuffHandler::CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (FBuffRestriction const& Restriction : IncomingBuffRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(BuffEvent))
		{
			return true;
		}
	}
	return false;
}

void UBuffHandler::AddOutgoingBuffRestriction(FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingBuffRestrictions.AddUnique(Restriction);
	}
}

void UBuffHandler::RemoveOutgoingBuffRestriction(FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingBuffRestrictions.Remove(Restriction);
	}
}

bool UBuffHandler::CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (FBuffRestriction const& Restriction : OutgoingBuffRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(BuffEvent))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion