// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffHandler.h"

#include "Buff.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"

UBuffHandler::UBuffHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBuffHandler::ApplyBuff(FBuffApplyEvent& BuffEvent)
{
	UBuff* AffectedBuff = FindExistingBuff(BuffEvent.BuffClass, BuffEvent.AppliedBy);
	if (AffectedBuff && !(AffectedBuff->GetDuplicable() || BuffEvent.DuplicateOverride))
	{
		AffectedBuff->ApplyEvent(BuffEvent);
	}
	else
	{
		CreateNewBuff(BuffEvent);
	}
	
	if (BuffEvent.Result.ActionTaken == EBuffApplyAction::NewBuff)
	{
		OnIncomingBuffApplied.Broadcast(BuffEvent);
	}
}

void UBuffHandler::RemoveBuff(FBuffRemoveEvent& RemoveEvent)
{
	EBuffType const BuffType = RemoveEvent.RemovedBuff->GetBuffType();
	switch (BuffType)
	{
		case EBuffType::Buff :
			RemoveEvent.Result = Buffs.Remove(RemoveEvent.RemovedBuff) > 0;
			break;
		case EBuffType::Debuff :
			RemoveEvent.Result = Debuffs.Remove(RemoveEvent.RemovedBuff) > 0;
			break;
		case EBuffType::HiddenBuff :
			RemoveEvent.Result = HiddenBuffs.Remove(RemoveEvent.RemovedBuff) > 0;
			break;
		default :
			RemoveEvent.Result = false;
			break;
	}

	if (RemoveEvent.Result)
	{
		RemoveEvent.RemovedBuff->ExpireBuff(RemoveEvent);
		OnIncomingBuffRemoved.Broadcast(RemoveEvent);

		//This is to allow replication one last time (replicating the remove event).
		RecentlyRemoved.Add(RemoveEvent.RemovedBuff);
		FTimerHandle RemoveTimer;
		FTimerDelegate const RemoveDel = FTimerDelegate::CreateUObject(this, &UBuffHandler::PostRemoveCleanup, RemoveEvent.RemovedBuff);
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, RemoveDel, 1.0f, false);
	}
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

void UBuffHandler::AddIncomingBuffRestriction(FBuffEventCondition const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingBuffConditions.AddUnique(Restriction);
	}
}

void UBuffHandler::RemoveIncomingBuffRestriction(FBuffEventCondition const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingBuffConditions.RemoveSingleSwap(Restriction);
	}
}

bool UBuffHandler::CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (FBuffEventCondition const& Condition : IncomingBuffConditions)
	{
		if (Condition.IsBound() && Condition.Execute(BuffEvent))
		{
			return true;
		}
	}
	return false;
}

void UBuffHandler::CreateNewBuff(FBuffApplyEvent& BuffEvent)
{
	UBuff* NewBuff = NewObject<UBuff>(GetOwner(), BuffEvent.BuffClass);
	NewBuff->InitializeBuff(BuffEvent, this);
	switch (NewBuff->GetBuffType())
	{
		case EBuffType::Buff :
			Buffs.Add(NewBuff);
			break;
		case EBuffType::Debuff :
			Debuffs.Add(NewBuff);
			break;
		case EBuffType::HiddenBuff :
			HiddenBuffs.Add(NewBuff);
			break;
		default :
			break;
	}
}

void UBuffHandler::SubscribeToIncomingBuffSuccess(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingBuffApplied.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromIncomingBuffSuccess(FBuffEventCallback const& Callback)
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

bool UBuffHandler::CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent)
{
	for (FBuffEventCondition const& Condition : OutgoingBuffConditions)
	{
		if (Condition.IsBound() && Condition.Execute(BuffEvent))
		{
			return true;
		}
	}
	return false;
}

void UBuffHandler::SuccessfulOutgoingBuffApplication(FBuffApplyEvent const& BuffEvent)
{
	if (BuffEvent.Result.ActionTaken == EBuffApplyAction::NewBuff)
	{
		OutgoingBuffs.Add(BuffEvent.Result.AffectedBuff);
	}
	OnOutgoingBuffApplied.Broadcast(BuffEvent);
}

void UBuffHandler::SuccessfulOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent)
{
	OutgoingBuffs.RemoveSingleSwap(RemoveEvent.RemovedBuff);
	OnOutgoingBuffRemoved.Broadcast(RemoveEvent);
}

void UBuffHandler::AddOutgoingBuffRestriction(FBuffEventCondition const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingBuffConditions.AddUnique(Restriction);
	}
}

void UBuffHandler::RemoveOutgoingBuffRestriction(FBuffEventCondition const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingBuffConditions.RemoveSingleSwap(Restriction);
	}
}

void UBuffHandler::SubscribeToOutgoingBuffSuccess(FBuffEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingBuffApplied.AddUnique(Callback);
	}
}

void UBuffHandler::UnsubscribeFromOutgoingBuffSuccess(FBuffEventCallback const& Callback)
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

//Replication

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

void UBuffHandler::NotifyOfReplicatedIncomingBuffApply(FBuffApplyEvent const& ReplicatedEvent)
{
	if (ReplicatedEvent.Result.ActionTaken == EBuffApplyAction::NewBuff)
	{
		switch (ReplicatedEvent.Result.AffectedBuff->GetBuffType())
		{
		case EBuffType::Buff :
			Buffs.Add(ReplicatedEvent.Result.AffectedBuff);
			break;
		case EBuffType::Debuff :
			Debuffs.Add(ReplicatedEvent.Result.AffectedBuff);
			break;
		case EBuffType::HiddenBuff :
			HiddenBuffs.Add(ReplicatedEvent.Result.AffectedBuff);
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
	if (ReplicatedEvent.Result.ActionTaken == EBuffApplyAction::NewBuff)
	{
		OutgoingBuffs.Add(ReplicatedEvent.Result.AffectedBuff);
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