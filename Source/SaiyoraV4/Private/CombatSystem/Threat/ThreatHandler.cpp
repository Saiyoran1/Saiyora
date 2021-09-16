﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "DamageHandler.h"

float UThreatHandler::GlobalHealingThreatModifier = .3f;

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
		UBuffHandler* BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		if (IsValid(BuffHandlerRef))
		{
			//TODO: Add restriction on buffs with immune threat controls.
		}
		DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->SubscribeToLifeStatusChanged(OwnerDeathCallback);
			DamageHandlerRef->SubscribeToIncomingDamageSuccess(ThreatFromDamageCallback);
		}
	}
}

void UThreatHandler::InitializeComponent()
{
	FadeCallback.BindDynamic(this, &UThreatHandler::OnTargetFadeStatusChanged);
	DeathCallback.BindDynamic(this, &UThreatHandler::OnTargetDied);
	OwnerDeathCallback.BindDynamic(this, &UThreatHandler::OnOwnerDied);
	ThreatFromDamageCallback.BindDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
	ThreatFromHealingCallback.BindDynamic(this, &UThreatHandler::OnTargetHealingTaken);
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

void UThreatHandler::OnOwnerDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor) || Actor != GetOwner() || New != ELifeStatus::Dead)
	{
		return;
	}
	for (int i = ThreatTable.Num() - 1; i > 0; i--)
	{
		if (IsValid(ThreatTable[i].Target))
		{
			RemoveFromThreatTable(ThreatTable[i].Target);
		}
	}
}

void UThreatHandler::OnOwnerDamageTaken(FDamagingEvent const& DamageEvent)
{
	if (!DamageEvent.Result.Success || !DamageEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Damage, DamageEvent.ThreatInfo.BaseThreat, DamageEvent.DamageInfo.AppliedBy,
		DamageEvent.DamageInfo.Source, DamageEvent.ThreatInfo.IgnoreRestrictions, DamageEvent.ThreatInfo.IgnoreModifiers,
		DamageEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnTargetHealingTaken(FHealingEvent const& HealingEvent)
{
	if (!HealingEvent.Result.Success || !HealingEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Healing, (HealingEvent.ThreatInfo.BaseThreat * GlobalHealingThreatModifier), HealingEvent.HealingInfo.AppliedBy,
		HealingEvent.HealingInfo.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
		HealingEvent.ThreatInfo.SourceModifier);
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

void UThreatHandler::NotifyAddedToThreatTable(AActor* Actor)
{
	if (TargetedBy.Contains(Actor))
	{
		return;
	}
	TargetedBy.Add(Actor);
	if (TargetedBy.Num() == 1)
	{
		UpdateCombatStatus();
	}
}

void UThreatHandler::NotifyRemovedFromThreatTable(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	if (TargetedBy.Remove(Actor) > 0 && TargetedBy.Num() == 0)
	{
		UpdateCombatStatus();
	}
}

FThreatEvent UThreatHandler::AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || BaseThreat <= 0.0f || !bCanEverReceiveThreat)
	{
		return Result;
	}
	
	//Target must either be alive, or not have a health component.
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return Result;
	}
	
	if (!AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return Result;
	}
	//Generator must either be alive, or not have a health component.
	UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->GetLifeStatus() != ELifeStatus::Alive)
	{
		return Result;
	}

	if (IsValid(GeneratorComponent->GetMisdirectTarget()))
	{
		//If we are misdirected to a different actor, that actor must also be alive or not have a health component.
		UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorComponent->GetMisdirectTarget());
		if (IsValid(MisdirectHealth) && MisdirectHealth->GetLifeStatus() != ELifeStatus::Alive)
		{
			//If the misdirected actor is dead, threat will come from the original generator.
			Result.AppliedBy = AppliedBy;
		}
		else
		{
			Result.AppliedBy = GeneratorComponent->GetMisdirectTarget();
		}
	}
	else
	{
		Result.AppliedBy = AppliedBy;
	}

	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	Result.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedBy);
	Result.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedTo);
	Result.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		TArray<FCombatModifier> Mods;
		GeneratorComponent->GetOutgoingThreatMods(Mods, Result);
		GetIncomingThreatMods(Mods, Result);
		if (SourceModifier.IsBound())
		{
			Mods.Add(SourceModifier.Execute(Result));
		}
		Result.Threat = FCombatModifier::ApplyModifiers(Mods, Result.Threat);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions)
	{
		if (GeneratorComponent->CheckOutgoingThreatRestricted(Result) || CheckIncomingThreatRestricted(Result))
		{
			return Result;
		}
	}

	int32 FoundIndex = ThreatTable.Num();
	bool bFound = false;
	for (; FoundIndex > 0; FoundIndex--)
	{
		if (ThreatTable[FoundIndex - 1].Target == Result.AppliedBy)
		{
			bFound = true;
			ThreatTable[FoundIndex - 1].Threat += Result.Threat;
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
		Result.bInitialThreat = true;
		AddToThreatTable(Result.AppliedBy, Result.Threat);
	}
	Result.bSuccess = true;

	return Result;
}

void UThreatHandler::UpdateCombatStatus()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (bInCombat)
	{
		if (ThreatTable.Num() == 0 && TargetedBy.Num() == 0)
		{
			bInCombat = false;
			OnCombatChanged.Broadcast(false);
		}
	}
	else
	{
		if (ThreatTable.Num() > 0 || TargetedBy.Num() > 0)
		{
			bInCombat = true;
			OnCombatChanged.Broadcast(true);
		}
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(bInCombat);
}

void UThreatHandler::AddToThreatTable(AActor* Actor, float const InitialThreat, int32 const InitialFixates,
	int32 const InitialBlinds)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Actor, InitialThreat, InitialFixates, InitialBlinds));
		UpdateTarget();
		UpdateCombatStatus();
	}
	else
	{
		for (FThreatTarget const& Target : ThreatTable)
		{
			if (Target.Target == Actor)
			{
				return;
			}
		}
		FThreatTarget const NewTarget = FThreatTarget(Actor, InitialThreat, InitialFixates, InitialBlinds);
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
	UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
	if (IsValid(TargetThreatHandler))
	{
		TargetThreatHandler->SubscribeToFadeStatusChanged(FadeCallback);
		TargetThreatHandler->NotifyAddedToThreatTable(GetOwner());
	}
	UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
	if (IsValid(TargetDamageHandler))
	{	
		TargetDamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		TargetDamageHandler->SubscribeToIncomingHealingSuccess(ThreatFromHealingCallback);
	}
}

void UThreatHandler::RemoveFromThreatTable(AActor* Actor)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->UnsubscribeFromFadeStatusChanged(FadeCallback);
				TargetThreatHandler->NotifyRemovedFromThreatTable(GetOwner());
			}
			UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
			if (IsValid(TargetDamageHandler))
			{	
				TargetDamageHandler->UnsubscribeFromLifeStatusChanged(DeathCallback);
				TargetDamageHandler->UnsubscribeFromIncomingHealingSuccess(ThreatFromHealingCallback);
			}
			if (i == ThreatTable.Num() - 1)
			{
				bAffectedTarget = true;
			}
			ThreatTable.RemoveAt(i);
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
	if (ThreatTable.Num() == 0)
	{
		UpdateCombatStatus();
	}
}

void UThreatHandler::OnTargetDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New)
{
	if (!IsValid(Actor) || New != ELifeStatus::Dead || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	RemoveFromThreatTable(Actor);
}

void UThreatHandler::SubscribeToFadeStatusChanged(FFadeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnFadeStatusChanged.AddUnique(Callback);
}

void UThreatHandler::UnsubscribeFromFadeStatusChanged(FFadeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnFadeStatusChanged.Remove(Callback);
}

void UThreatHandler::OnTargetFadeStatusChanged(AActor* Actor, bool const FadeStatus)
{
	if (!IsValid(Actor))
	{
		return;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
		{
			bool AffectedTarget = false;
			if (FadeStatus)
			{
				if (ThreatTable[i].Blinds == 0)
				{
					while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
					{
						ThreatTable.Swap(i, i - 1);
						if (!AffectedTarget && i == ThreatTable.Num() - 1)
						{
							AffectedTarget = true;
						}
						i--;
					}
				}
			}
			else
			{
				if (ThreatTable[i].Blinds == 0)
				{
					while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
					{
						ThreatTable.Swap(i, i + 1);
						if (!AffectedTarget && i + 1 == ThreatTable.Num() - 1)
						{
							AffectedTarget = true;
						}
						i++;
					}
				}
			}
			if (AffectedTarget)
			{
				UpdateTarget();
			}
			return;
		}
	}
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

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
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
	//Can not have multiple fixates per buff.
	if (Fixates.Contains(Source))
	{
		return;
	}
	Fixates.Add(Source, Target);
	
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bFound = true;
			ThreatTable[i].Fixates++;
			//If this was the first fixate for this target, possible reorder on threat.
			if (ThreatTable[i].Fixates == 1)
			{
				while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
				{
					ThreatTable.Swap(i, i + 1);
					if (!bAffectedTarget && i + 1 == ThreatTable.Num() - 1)
					{
						bAffectedTarget = true;
					}
					i++;
				}
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
	//If this unit was not in the threat table, need to add them.
	if (!bFound)
	{
		AddToThreatTable(Target, 0.0f, 1);
	}
}

void UThreatHandler::RemoveFixate(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	AActor* AffectedActor = nullptr;
	bool const Removed = Fixates.RemoveAndCopyValue(Source, AffectedActor);
	if (!Removed || !IsValid(AffectedActor))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == AffectedActor)
		{
			if (ThreatTable[i].Fixates > 0)
			{
				ThreatTable[i].Fixates--;
				//If we removed the last fixate for this target, possibly reorder on threat.
				if (ThreatTable[i].Fixates == 0)
				{
					while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
					{
						ThreatTable.Swap(i, i - 1);
						//If the last index was swapped, this will change current target.
						if (!bAffectedTarget && i == ThreatTable.Num() - 1)
						{
							bAffectedTarget = true;
						}
						i--;
					}
				}
			}
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
}

void UThreatHandler::AddBlind(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
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
	//Can not have multiple blinds from the same buff.
	if (Blinds.Contains(Source))
	{
		return;
	}
	Blinds.Add(Source, Target);
	
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bFound = true;
			ThreatTable[i].Blinds++;
			//If this is the first Blind for the unit, possibly reorder on threat.
			if (ThreatTable[i].Blinds == 1)
			{
				while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
				{
					ThreatTable.Swap(i, i - 1);
					//If the last index was affected by the swap, this will change current target.
					if (!bAffectedTarget && i == ThreatTable.Num() - 1)
					{
						bAffectedTarget = true;
					}
					i--;
				}
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
	if (!bFound)
	{
		AddToThreatTable(Target, 0.0f, 0, 1);
	}
}

void UThreatHandler::RemoveBlind(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	AActor* AffectedActor = nullptr;
	bool const Removed = Blinds.RemoveAndCopyValue(Source, AffectedActor);
	if (!Removed || !IsValid(AffectedActor))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == AffectedActor)
		{
			if (ThreatTable[i].Blinds > 0)
			{
				ThreatTable[i].Blinds--;
				//If we went from non-zero Blinds to zero Blinds, possibly have to reorder on threat.
				if (ThreatTable[i].Blinds == 0)
				{
					while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
					{
						ThreatTable.Swap(i, i + 1);
						//If the last index was swapped, then this will change the current target.
						if (!bAffectedTarget && i == ThreatTable.Num() - 1)
						{
							bAffectedTarget = true;
						}
						i++;
					}
				}
			}
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
}

void UThreatHandler::AddFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || !IsValid(Source))
	{
		return;
	}
	if (Fades.Contains(Source))
	{
		return;
	}
	Fades.Add(Source);
	if (Fades.Num() == 1)
	{
		OnFadeStatusChanged.Broadcast(GetOwner(), true);
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || Fades.Num() == 0)
	{
		return;
	}
	int32 const Removed = Fades.Remove(Source);
	if (Removed != 0 && Fades.Num() == 0)
	{
		OnFadeStatusChanged.Broadcast(GetOwner(), true);
	}
}
