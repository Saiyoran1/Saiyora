// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "DamageHandler.h"

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		if (IsValid(BuffHandlerRef))
		{
			BuffRemovalCallback.BindDynamic(this, &UThreatHandler::RemoveThreatControlsOnBuffRemoved);
			BuffHandlerRef->SubscribeToIncomingBuffRemove(BuffRemovalCallback);
		}
	}
}

void UThreatHandler::InitializeComponent()
{
	//TODO: Threat Handler initialization.
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

void UThreatHandler::GetIncomingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event)
{
	for (FThreatModCondition const& Modifier : IncomingThreatMods)
	{
		if (Modifier.IsBound())
		{
			OutMods.Add(Modifier.Execute(Event));
		}
	}
}

void UThreatHandler::GetOutgoingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event)
{
	for (FThreatModCondition const& Modifier : OutgoingThreatMods)
	{
		if (Modifier.IsBound())
		{
			OutMods.Add(Modifier.Execute(Event));
		}
	}
}

bool UThreatHandler::CheckIncomingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatCondition const& Restriction : IncomingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

bool UThreatHandler::CheckOutgoingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatCondition const& Restriction : OutgoingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

void UThreatHandler::AddThreat(FThreatEvent& Event)
{
	if (Event.Threat <= 0.0f)
	{
		return;
	}
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Event.AppliedBy, Event.Threat));
		Event.bInitialThreat = true;
		UpdateTarget();
		EnterCombat();
	}
	else
	{
		int32 FoundIndex = ThreatTable.Num();
		bool bFound = false;
		for (; FoundIndex > 0; FoundIndex--)
		{
			if (ThreatTable[FoundIndex - 1].Target == Event.AppliedBy)
			{
				bFound = true;
				ThreatTable[FoundIndex - 1].Threat += Event.Threat;
				while (FoundIndex < ThreatTable.Num() && ThreatTable[FoundIndex] < ThreatTable[FoundIndex - 1])
				{
					ThreatTable.Swap(FoundIndex, FoundIndex - 1);
					FoundIndex++;
				}
				break;
			}
		}
		if (bFound && FoundIndex == ThreatTable.Num())
		{
			UpdateTarget();
		}
		if (!bFound)
		{
			FThreatTarget const NewTarget = FThreatTarget(Event.AppliedBy, Event.Threat);
			int32 InsertIndex = 0;
			for (; InsertIndex < ThreatTable.Num(); InsertIndex++)
			{
				if (!(ThreatTable[InsertIndex] < NewTarget))
				{
					break;
				}
			}
			ThreatTable.Insert(NewTarget, InsertIndex);
			Event.bInitialThreat = true;
			if (InsertIndex == ThreatTable.Num() - 1)
			{
				UpdateTarget();
			}
		}
	}
	Event.bSuccess = true;
}

void UThreatHandler::EnterCombat()
{
	if (bInCombat)
	{
		return;
	}
	if (ThreatTable.Num() == 0)
	{
		LeaveCombat();
		return;
	}
	bInCombat = true;
	OnCombatChanged.Broadcast(true);
}

void UThreatHandler::LeaveCombat()
{
	if (!bInCombat)
	{
		return;
	}
	if (ThreatTable.Num() != 0)
	{
		EnterCombat();
		return;
	}
	bInCombat = false;
	OnCombatChanged.Broadcast(false);
}

void UThreatHandler::OnRep_CombatStatus()
{
	OnCombatChanged.Broadcast(bInCombat);
}

void UThreatHandler::UpdateTarget()
{
	AActor* Previous = CurrentTarget;
	if (ThreatTable.Num() == 0)
	{
		CurrentTarget = nullptr;
		if (IsValid(Previous))
		{
			OnTargetChanged.Broadcast(Previous, CurrentTarget);
		}
		return;
	}
	CurrentTarget = ThreatTable[ThreatTable.Num() - 1].Target;
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::OnRep_CurrentTarget(AActor* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

void UThreatHandler::RemoveThreatControlsOnBuffRemoved(FBuffRemoveEvent const& RemoveEvent)
{
	if (!RemoveEvent.Result)
	{
		return;
	}
	FGameplayTagContainer RemovedBuffTags;
	RemoveEvent.RemovedBuff->GetBuffTags(RemovedBuffTags);
	AActor* FixateRemovedFrom = nullptr;
	AActor* BlindRemovedFrom = nullptr;
	if (RemovedBuffTags.HasTagExact(FixateTag()))
	{
		Fixates.RemoveAndCopyValue(RemoveEvent.RemovedBuff, FixateRemovedFrom);
	}
	if (RemovedBuffTags.HasTagExact(BlindTag()))
	{
		Blinds.RemoveAndCopyValue(RemoveEvent.RemovedBuff, BlindRemovedFrom);
	}
	if (IsValid(BlindRemovedFrom) || IsValid(FixateRemovedFrom))
	{
		bool AffectedCurrentTarget = false;
		bool FixateComplete = !IsValid(FixateRemovedFrom);
		bool BlindComplete = !IsValid(BlindRemovedFrom);
		int32 FixateIndex = -1;
		int32 BlindIndex = -1;
		for (int i = 0; i < ThreatTable.Num(); i++)
		{
			if (!FixateComplete && ThreatTable[i].Target == FixateRemovedFrom && ThreatTable[i].Fixates > 0)
			{
				ThreatTable[i].Fixates--;
				FixateComplete = true;
				if (ThreatTable[i].Fixates == 0)
				{
					FixateIndex = i;
				}
			}
			if (!BlindComplete && ThreatTable[i].Target == BlindRemovedFrom && ThreatTable[i].Blinds > 0)
			{
				ThreatTable[i].Blinds--;
				BlindComplete = true;
				if (ThreatTable[i].Blinds == 0)
				{
					BlindIndex = i;
				}
			}
			if (FixateComplete && BlindComplete)
			{
				break;
			}
		}
		if (FixateIndex == BlindIndex && FixateIndex != -1)
		{
			//TODO: Only one target affected by both things.
			return;
		}
		if (FixateIndex != -1)
		{
			//Fixate target, different from blind target.
			while (FixateIndex > 0 && ThreatTable[FixateIndex] < ThreatTable[FixateIndex - 1])
			{
				//If we swap with the blind index, make sure to adjust blind index to compensate.
				if (FixateIndex - 1 == BlindIndex && BlindIndex != -1)
				{
					BlindIndex = FixateIndex;
				}
				ThreatTable.Swap(FixateIndex, FixateIndex - 1);
				if (!AffectedCurrentTarget && FixateIndex == ThreatTable.Num() - 1)
				{
					AffectedCurrentTarget = true;
				}
				FixateIndex--;
			}
		}
		if (BlindIndex != -1)
		{
			//Blind target, different from fixate target.
			while (BlindIndex < ThreatTable.Num() - 1 && ThreatTable[BlindIndex + 1] < ThreatTable[BlindIndex])
			{
				ThreatTable.Swap(BlindIndex, BlindIndex + 1);
				if (!AffectedCurrentTarget && BlindIndex + 1 == ThreatTable.Num() - 1)
				{
					AffectedCurrentTarget = true;
				}
				BlindIndex++;
			}
		}
		if (AffectedCurrentTarget)
		{
			UpdateTarget();
		}
	}
	if (RemovedBuffTags.HasTagExact(FadeTag()))
	{
		if (Fades.Num() != 0)
		{
			if (Fades.Remove(RemoveEvent.RemovedBuff) > 0 && Fades.Num() == 0)
			{
				OnFadeStatusChanged.Broadcast(false);
			}
		}
	}
	if (RemovedBuffTags.HasTagExact(MisdirectTag()))
	{
		//TODO: Remove misdirect.
	}
}

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (!IsValid(Target) || !IsValid(Source) || !IsValid(BuffHandlerRef) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return;
	}
	UDamageHandler* HealthComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(Target);
	if (IsValid(HealthComponent) && HealthComponent->GetLifeStatus() != ELifeStatus::Alive)
	{
		return;
	}
	if (Fixates.Contains(Source))
	{
		return;
	}
	Fixates.Add(Source, Target);
	
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Target, 0.0f, 1));
		UpdateTarget();
		EnterCombat();
		return;
	}
	
	int32 FoundIndex = ThreatTable.Num();
	bool bFound = false;
	for (; FoundIndex > 0; FoundIndex--)
	{
		if (ThreatTable[FoundIndex - 1].Target == Target)
		{
			bFound = true;
			ThreatTable[FoundIndex - 1].Fixates++;
			while (FoundIndex < ThreatTable.Num() && ThreatTable[FoundIndex] < ThreatTable[FoundIndex - 1])
			{
				ThreatTable.Swap(FoundIndex, FoundIndex - 1);
				FoundIndex++;
			}
			break;
		}
	}
	if (bFound && FoundIndex == ThreatTable.Num())
	{
		UpdateTarget();
	}
	if (!bFound)
	{
		FThreatTarget const NewTarget = FThreatTarget(Target, 0.0f, 1);
		int32 InsertIndex = 0;
		for (; InsertIndex < ThreatTable.Num(); InsertIndex++)
		{
			if (!(ThreatTable[InsertIndex] < NewTarget))
			{
				break;
			}
		}
		ThreatTable.Insert(NewTarget, InsertIndex);
		if (InsertIndex == ThreatTable.Num() - 1)
		{
			UpdateTarget();
		}
	}
}
