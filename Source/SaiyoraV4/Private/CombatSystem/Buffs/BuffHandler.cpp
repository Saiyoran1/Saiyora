#include "BuffHandler.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "AbilityComponent.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"

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
	if (GetOwnerRole() == ROLE_Authority)
	{
		PlaneComponentRef = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
		DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
		ThreatHandlerRef = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
		MovementComponentRef = nullptr; //TODO: Get Movement Component Ref.
		CcHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	}
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
	Event.OriginPlane = IsValid(GeneratorPlane) ? GeneratorPlane->GetCurrentPlane() : ESaiyoraPlane::None;
    Event.AppliedTo = GetOwner();
	Event.TargetPlane = IsValid(PlaneComponentRef) ? PlaneComponentRef->GetCurrentPlane() : ESaiyoraPlane::None;
	Event.AppliedXPlane = UPlaneComponent::CheckForXPlane(Event.OriginPlane, Event.TargetPlane);
    Event.Source = Source;
    Event.BuffClass = BuffClass;
	Event.CombatParams = BuffParams;
	UCombatAbility* AbilitySource = Cast<UCombatAbility>(Source);
	if (IsValid(AbilitySource) && AbilitySource->GetHandler()->GetOwner() == Event.AppliedBy && AbilitySource->GetHandler()->GetOwner() == Event.AppliedTo)
	{
		Event.PredictionID = AbilitySource->GetPredictionID();
	}

    if (!IgnoreRestrictions && (CheckIncomingBuffRestricted(Event) || (IsValid(GeneratorBuff) && GeneratorBuff->CheckOutgoingBuffRestricted(Event))))
    {
    	Event.ActionTaken = EBuffApplyAction::Failed;
    	return Event;
    }

	UBuff* AffectedBuff = FindExistingBuff(BuffClass, true, AppliedBy);
	if (IsValid(AffectedBuff) && !AffectedBuff->IsDuplicable() && !DuplicateOverride)
	{
		AffectedBuff->ApplyEvent(Event, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
	else
	{
		Event.AffectedBuff = NewObject<UBuff>(GetOwner(), Event.BuffClass);
		Event.AffectedBuff->InitializeBuff(Event, this, IgnoreRestrictions, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
    return Event;
}

void UBuffHandler::NotifyOfNewIncomingBuff(FBuffApplyEvent const& ApplicationEvent)
{
	if (!IsValid(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	TArray<UBuff*>* BuffArray;
	switch (ApplicationEvent.AffectedBuff->GetBuffType())
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
		return;
	}
	if (BuffArray->Contains(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	BuffArray->Add(ApplicationEvent.AffectedBuff);
	OnIncomingBuffApplied.Broadcast(ApplicationEvent);
	if (IsValid(ApplicationEvent.AffectedBuff->GetAppliedBy()) && ApplicationEvent.AffectedBuff->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UBuffHandler* GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(ApplicationEvent.AffectedBuff->GetAppliedBy());
		if (IsValid(GeneratorBuff))
		{
			GeneratorBuff->NotifyOfNewOutgoingBuff(ApplicationEvent);
		}
	}
}

void UBuffHandler::NotifyOfNewOutgoingBuff(FBuffApplyEvent const& ApplicationEvent)
{
	if (!IsValid(ApplicationEvent.AffectedBuff) || OutgoingBuffs.Contains(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	OutgoingBuffs.Add(ApplicationEvent.AffectedBuff);
	OnOutgoingBuffApplied.Broadcast(ApplicationEvent);
}

#pragma endregion
#pragma region Removal

FBuffRemoveEvent UBuffHandler::RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Buff) || Buff->GetAppliedTo() != GetOwner())
	{
		return FBuffRemoveEvent();
	}
	return Buff->TerminateBuff(ExpireReason);
}

void UBuffHandler::NotifyOfIncomingBuffRemoval(FBuffRemoveEvent const& RemoveEvent)
{
	if (!IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	TArray<UBuff*>* BuffArray;
	switch (RemoveEvent.RemovedBuff->GetBuffType())
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
		return;
	}
	if (BuffArray->Remove(RemoveEvent.RemovedBuff) > 0)
	{
		OnIncomingBuffRemoved.Broadcast(RemoveEvent);
		//This is to allow replication one last time (replicating the remove event).
		RecentlyRemoved.Add(RemoveEvent.RemovedBuff);
		FTimerHandle RemoveTimer;
		FTimerDelegate const RemoveDel = FTimerDelegate::CreateUObject(this, &UBuffHandler::PostRemoveCleanup, RemoveEvent.RemovedBuff);
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, RemoveDel, 1.0f, false);
		
		if (IsValid(RemoveEvent.RemovedBuff->GetAppliedBy()) && RemoveEvent.RemovedBuff->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(RemoveEvent.RemovedBuff->GetAppliedBy());
			if (IsValid(BuffGenerator))
			{
				BuffGenerator->NotifyOfOutgoingBuffRemoval(RemoveEvent);
			}
		}
	}
}

void UBuffHandler::NotifyOfOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent)
{
	if (OutgoingBuffs.Remove(RemoveEvent.RemovedBuff) > 0)
	{
		OnOutgoingBuffRemoved.Broadcast(RemoveEvent);
	}
}

void UBuffHandler::PostRemoveCleanup(UBuff* Buff)
{
	RecentlyRemoved.Remove(Buff);	
}

#pragma endregion 
#pragma region Get Buffs

void UBuffHandler::GetBuffsOfClass(TSubclassOf<UBuff> const BuffClass, TArray<UBuff*>& OutBuffs) const
{
	if (!IsValid(BuffClass))
	{
		return;
	}
	TArray<UBuff*> const* BuffArray;
	switch (BuffClass.GetDefaultObject()->GetBuffType())
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
			return;
	}
	for (UBuff* Buff : *BuffArray)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass)
		{
			OutBuffs.Add(Buff);
		}
	}
}

void UBuffHandler::GetBuffsAppliedByActor(AActor* Actor, TArray<UBuff*>& OutBuffs) const
{
	if (!IsValid(Actor))
	{
		return;
	}
	for (UBuff* Buff : Buffs)
	{
		if (IsValid(Buff) && Buff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(Buff);
		}
	}
	for (UBuff* Debuff : Debuffs)
	{
		if (IsValid(Debuff) && Debuff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(Debuff);
		}
	}
	for (UBuff* HiddenBuff : HiddenBuffs)
	{
		if (IsValid(HiddenBuff) && HiddenBuff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(HiddenBuff);
		}
	}
}

void UBuffHandler::GetOutgoingBuffsOfClass(TSubclassOf<UBuff> const BuffClass, TArray<UBuff*>& OutBuffs) const
{
	if (!IsValid(BuffClass))
	{
		return;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass)
		{
			OutBuffs.Add(Buff);
		}
	}
}

void UBuffHandler::GetBuffsAppliedToActor(AActor* Target, TArray<UBuff*>& OutBuffs) const
{
	if (!IsValid(Target))
	{
		return;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetAppliedTo() == Target)
		{
			OutBuffs.Add(Buff);
		}
	}
}

UBuff* UBuffHandler::FindExistingOutgoingBuff(TSubclassOf<UBuff> const BuffClass, bool const bSpecificTarget,
	AActor* BuffTarget) const
{
	if (!IsValid(BuffClass) || (bSpecificTarget && !IsValid(BuffTarget)))
	{
		return nullptr;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass && (!bSpecificTarget || Buff->GetAppliedTo() == BuffTarget))
		{
			return Buff;
		}
	}
	return nullptr;
}

UBuff* UBuffHandler::FindExistingBuff(TSubclassOf<UBuff> const BuffClass, bool const bSpecificOwner, AActor* BuffOwner) const
{
	if (!IsValid(BuffClass) || (bSpecificOwner && !IsValid(BuffOwner)))
	{
		return nullptr;
	}
	TArray<UBuff*> const* BuffArray;
	switch (BuffClass.GetDefaultObject()->GetBuffType())
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
			return nullptr;
	}
	for (UBuff* Buff : *BuffArray)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass && (!bSpecificOwner || Buff->GetAppliedBy() == BuffOwner))
		{
			return Buff;
		}
	}
	return nullptr;
}

#pragma endregion 
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

void UBuffHandler::AddIncomingBuffRestriction(UBuff* Source, FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingBuffRestrictions.Add(Source, Restriction);
	}
}

void UBuffHandler::RemoveIncomingBuffRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingBuffRestrictions.Remove(Source);
	}
}

bool UBuffHandler::CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (TTuple<UBuff*, FBuffRestriction> const& Restriction : IncomingBuffRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(BuffEvent))
		{
			return true;
		}
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	if (!BuffEvent.BuffClass.GetDefaultObject()->CanBeAppliedWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return true;
	}
	for (FGameplayTag const Tag : BuffTags)
	{
		if (Tag.MatchesTagExact(UDamageHandler::GenericDamageTag()) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveDamage()))
		{
			return true;
		}
		if (Tag.MatchesTagExact(UDamageHandler::GenericHealingTag()) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveHealing()))
		{
			return true;
		}
		if (Tag.MatchesTag(UStatHandler::GenericStatTag()) && !Tag.MatchesTagExact(UStatHandler::GenericStatTag()) && (!IsValid(StatHandlerRef) || !StatHandlerRef->IsStatModifiable(Tag)))
		{
			return true;
		}
		if (Tag.MatchesTag(UCrowdControlHandler::GenericCrowdControlTag()) && !Tag.MatchesTagExact(UCrowdControlHandler::GenericCrowdControlTag()) && (!IsValid(CcHandlerRef) || CcHandlerRef->IsImmuneToCrowdControl(Tag)))
		{
			return true;
		}
		if (Tag.MatchesTag(UThreatHandler::GenericThreatTag()) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->HasThreatTable()))
		{
			return true;
		}
		if (Tag.MatchesTagExact(USaiyoraMovementComponent::GenericExternalMovementTag()) && !IsValid(MovementComponentRef))
		{
			return true;
		}
	}
	return false;
}

void UBuffHandler::AddOutgoingBuffRestriction(UBuff* Source, FBuffRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingBuffRestrictions.Add(Source, Restriction);
	}
}

void UBuffHandler::RemoveOutgoingBuffRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingBuffRestrictions.Remove(Source);
	}
}

bool UBuffHandler::CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (TTuple<UBuff*, FBuffRestriction> const& Restriction : OutgoingBuffRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(BuffEvent))
		{
			return true;
		}
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (FGameplayTag const Tag : BuffTags)
	{
		if (Tag.MatchesTag(UThreatHandler::GenericThreatTag()) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->CanBeInThreatTable()))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion