// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();
}

void UThreatHandler::InitializeComponent()
{
	//TODO: Threat Handler initialization.
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
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
