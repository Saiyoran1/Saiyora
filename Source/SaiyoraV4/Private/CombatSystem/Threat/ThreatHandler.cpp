// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "DamageHandler.h"
#include "SaiyoraBuffLibrary.h"

float UThreatHandler::GlobalHealingThreatModifier = .3f;
float UThreatHandler::GlobalTauntThreatPercentage = 1.2f;

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
	VanishCallback.BindDynamic(this, &UThreatHandler::OnTargetVanished);
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

void UThreatHandler::OnTargetVanished(AActor* Actor)
{
	if (IsValid(Actor))
	{
		RemoveFromThreatTable(Actor);
	}
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

float UThreatHandler::GetThreatLevel(AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target))
	{
		return 0.0f;
	}
	for (FThreatTarget const& ThreatEntry : ThreatTable)
	{
		if (IsValid(ThreatEntry.Target) && ThreatEntry.Target == Target)
		{
			return ThreatEntry.Threat;
		}
	}
	return 0.0f;
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
		AddToThreatTable(Result.AppliedBy, Result.Threat, GeneratorComponent->HasActiveFade());
	}
	Result.bSuccess = true;

	return Result;
}

void UThreatHandler::RemoveThreat(float const Amount, AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bCanEverReceiveThreat)
	{
		return;
	}
	bool bAffectedTarget = false;
	bool bFound = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == AppliedBy)
		{
			bFound = true;
			ThreatTable[i].Threat = FMath::Max(0.0f, ThreatTable[i].Threat - Amount);
			while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
			{
				ThreatTable.Swap(i, i - 1);
				if (!bAffectedTarget && i == ThreatTable.Num() - 1)
				{
					bAffectedTarget = true;
				}
				i--;
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
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

void UThreatHandler::AddToThreatTable(AActor* Actor, float const InitialThreat, bool const Faded, UBuff* InitialFixate,
	UBuff* InitialBlind)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Actor, InitialThreat, Faded, InitialFixate, InitialBlind));
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
		FThreatTarget const NewTarget = FThreatTarget(Actor, InitialThreat, Faded, InitialFixate, InitialBlind);
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
		TargetThreatHandler->SubscribeToVanished(VanishCallback);
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
				TargetThreatHandler->UnsubscribeFromVanished(VanishCallback);
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor) || New != ELifeStatus::Dead)
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

void UThreatHandler::SubscribeToVanished(FVanishCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnVanished.AddUnique(Callback);
}

void UThreatHandler::UnsubscribeFromVanished(FVanishCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnVanished.Remove(Callback);
}

void UThreatHandler::Taunt(AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bCanEverReceiveThreat)
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return;
	}
	float const InitialThreat = GetThreatLevel(AppliedBy);
	float HighestThreat = 0.0f;
	for (FThreatTarget const& Target : ThreatTable)
	{
		if (Target.Threat > HighestThreat)
		{
			HighestThreat = Target.Threat;
		}
	}
	HighestThreat *= GlobalTauntThreatPercentage;
	AddThreat(EThreatType::Absolute, FMath::Max(0.0f, HighestThreat - InitialThreat), AppliedBy, nullptr, true, true, FThreatModCondition());
}

void UThreatHandler::DropThreat(AActor* Target, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !bCanEverReceiveThreat)
	{
		return;
	}
	float const DropThreat = GetThreatLevel(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(DropThreat, Target);
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
			ThreatTable[i].Faded = FadeStatus;
			if (FadeStatus)
			{
				if (ThreatTable[i].Blinds.Num() == 0)
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
				if (ThreatTable[i].Blinds.Num() == 0)
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
			int32 const PreviousFixates = ThreatTable[i].Fixates.Num();
			ThreatTable[i].Fixates.AddUnique(Source);
			//If this was the first fixate for this target, possible reorder on threat.
			if (PreviousFixates == 0 && ThreatTable[i].Fixates.Num() == 1)
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
		AddToThreatTable(Target, 0.0f, GeneratorComponent->HasActiveFade(), Source);
	}
}

void UThreatHandler::RemoveFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			if (ThreatTable[i].Fixates.Num() > 0)
			{
				ThreatTable[i].Fixates.Remove(Source);
				//If we removed the last fixate for this target, possibly reorder on threat.
				if (ThreatTable[i].Fixates.Num() == 0)
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
			int32 const PreviousBlinds = ThreatTable[i].Blinds.Num();
			ThreatTable[i].Blinds.AddUnique(Source);
			//If this is the first Blind for the unit, possibly reorder on threat.
			if (PreviousBlinds == 0 && ThreatTable[i].Blinds.Num() == 1)
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
		AddToThreatTable(Target, 0.0f, GeneratorComponent->HasActiveFade(), nullptr, Source);
	}
}

void UThreatHandler::RemoveBlind(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			if (ThreatTable[i].Blinds.Num() > 0)
			{
				ThreatTable[i].Blinds.Remove(Source);
				//If we went from non-zero Blinds to zero Blinds, possibly have to reorder on threat.
				if (ThreatTable[i].Blinds.Num() == 0)
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

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	OnVanished.Broadcast(GetOwner());
	//This only clears us from actors that have us in their threat table. This does NOT clear our threat table. This shouldn't matter, as if an NPC vanishes its unlikely the player will care.
	//This only has ramifications if NPCs can possibly attack each other.
}

void UThreatHandler::TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetThreatLevel(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, true, true, FThreatModCondition());
}