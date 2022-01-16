#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "PlaneComponent.h"
#include "FactionComponent.h"

float UThreatHandler::GlobalHealingThreatModifier = 0.3f;
float UThreatHandler::GlobalTauntThreatPercentage = 1.2f;

#pragma region Initialization

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::InitializeComponent()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OwnerLifeStatusCallback.BindDynamic(this, &UThreatHandler::OnOwnerLifeStatusChanged);
		ThreatFromDamageCallback.BindDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
		TargetLifeStatusCallback.BindDynamic(this, &UThreatHandler::OnTargetLifeStatusChanged);
		ThreatFromIncomingHealingCallback.BindDynamic(this, &UThreatHandler::OnTargetHealingTaken);
		ThreatFromOutgoingHealingCallback.BindDynamic(this, &UThreatHandler::OnTargetHealingDone);
	}
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();
	
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Threat Handler."), *GetOwner()->GetActorLabel());
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		FactionCompRef = ISaiyoraCombatInterface::Execute_GetFactionComponent(GetOwner());
		checkf(IsValid(FactionCompRef), TEXT("%s does not have a valid Faction Component, which Threat Handler depends on."), *GetOwner()->GetActorLabel());
		DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		if (IsValid(DamageHandlerRef))
		{
			if (DamageHandlerRef->HasHealth())
			{
				DamageHandlerRef->SubscribeToLifeStatusChanged(OwnerLifeStatusCallback);
			}
			if (DamageHandlerRef->CanEverReceiveDamage())
			{
				DamageHandlerRef->SubscribeToIncomingDamage(ThreatFromDamageCallback);
			}
		}
		PlaneCompRef = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

void UThreatHandler::OnOwnerDamageTaken(FDamagingEvent const& DamageEvent)
{
	if (!DamageEvent.Result.Success || !DamageEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Damage, DamageEvent.ThreatInfo.BaseThreat, DamageEvent.Info.AppliedBy,
		DamageEvent.Info.Source, DamageEvent.ThreatInfo.IgnoreRestrictions, DamageEvent.ThreatInfo.IgnoreModifiers,
		DamageEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnOwnerLifeStatusChanged(AActor* Actor, ELifeStatus const PreviousStatus,
	ELifeStatus const NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		ClearThreatTable();
	}
}

#pragma endregion 
#pragma region Threat

FThreatEvent UThreatHandler::AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
                                       UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	if (!bHasThreatTable || BaseThreat < 0.0f || (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead()))
	{
		return Result;		
	}
	if (!IsValid(AppliedBy) || !AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorThreat) || !GeneratorThreat->CanBeInThreatTable())
	{
		return Result;
	}
	UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->IsDead())
	{
		return Result;
	}
	UFactionComponent* GeneratorFaction = ISaiyoraCombatInterface::Execute_GetFactionComponent(AppliedBy);
	if (!IsValid(GeneratorFaction) || GeneratorFaction->GetCurrentFaction() == FactionCompRef->GetCurrentFaction())
	{
		return Result;
	}

	bool bUseMisdirectTarget = false;
	if (IsValid(GeneratorThreat->GetMisdirectTarget()) && GeneratorThreat->GetMisdirectTarget()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UThreatHandler* MisdirectThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(GeneratorThreat->GetMisdirectTarget());
		if (IsValid(MisdirectThreat) && MisdirectThreat->CanBeInThreatTable())
		{
			UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorThreat->GetMisdirectTarget());
			if (!IsValid(MisdirectHealth) || !MisdirectHealth->IsDead())
			{
				UFactionComponent* MisdirectFaction = ISaiyoraCombatInterface::Execute_GetFactionComponent(GeneratorThreat->GetMisdirectTarget());
				if (IsValid(MisdirectFaction) && MisdirectFaction->GetCurrentFaction() != FactionCompRef->GetCurrentFaction())
				{
					bUseMisdirectTarget = true;
				}
			}
		}
	}
	
	Result.AppliedBy = bUseMisdirectTarget ? GeneratorThreat->GetMisdirectTarget() : AppliedBy;
	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	UPlaneComponent* GeneratorPlane = ISaiyoraCombatInterface::Execute_GetPlaneComponent(AppliedBy);
	Result.AppliedByPlane = IsValid(GeneratorPlane) ? GeneratorPlane->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedToPlane = IsValid(PlaneCompRef) ? PlaneCompRef->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedXPlane = UPlaneComponent::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		Result.Threat = GeneratorThreat->GetModifiedOutgoingThreat(Result, SourceModifier);
		Result.Threat = GetModifiedIncomingThreat(Result);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions)
	{
		if (GeneratorThreat->CheckOutgoingThreatRestricted(Result) || CheckIncomingThreatRestricted(Result))
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
		AddToThreatTable(FThreatTarget(Result.AppliedBy, Result.Threat, GeneratorThreat->HasActiveFade()));
	}
	Result.bSuccess = true;

	return Result;
}

void UThreatHandler::AddToThreatTable(FThreatTarget const& NewTarget)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(NewTarget.Target) || IsActorInThreatTable(NewTarget.Target))
	{
		return;
	}
	if (!NewTarget.Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(NewTarget.Target);
	if (!IsValid(TargetThreatHandler) || !TargetThreatHandler->CanBeInThreatTable())
	{
		return;
	}
	UFactionComponent* TargetFactionComp = ISaiyoraCombatInterface::Execute_GetFactionComponent(NewTarget.Target);
	if (!IsValid(TargetFactionComp) || TargetFactionComp->GetCurrentFaction() == FactionCompRef->GetCurrentFaction())
	{
		return;
	}
	UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(NewTarget.Target);
	if (IsValid(TargetDamageHandler) && TargetDamageHandler->IsDead())
	{
		return;
	}
	bool bShouldUpdateTarget = false;
	bool bShouldUpdateCombat = false;
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(NewTarget);
		bShouldUpdateTarget = true;
		bShouldUpdateCombat = true;
	}
	else
	{
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
			bShouldUpdateTarget = true;
		}
	}
	if (IsValid(TargetDamageHandler))
	{
		TargetDamageHandler->SubscribeToOutgoingHealing(ThreatFromOutgoingHealingCallback);
		if (TargetDamageHandler->HasHealth())
		{
			TargetDamageHandler->SubscribeToLifeStatusChanged(TargetLifeStatusCallback);
		}
		if (TargetDamageHandler->CanEverReceiveHealing())
		{
			TargetDamageHandler->SubscribeToIncomingHealing(ThreatFromIncomingHealingCallback);
		}
	}
	if (bShouldUpdateTarget)
	{
		UpdateTarget();
	}
	if (bShouldUpdateCombat)
	{
		UpdateCombat();
	}
	TargetThreatHandler->NotifyOfTargeting(this);
}

void UThreatHandler::NotifyOfTargeting(UThreatHandler* TargetingComponent)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(TargetingComponent) || TargetedBy.Contains(TargetingComponent->GetOwner()))
	{
		return;
	}
	TargetedBy.Add(TargetingComponent->GetOwner());
	if (TargetedBy.Num() == 1)
	{
		UpdateCombat();
	}
	if (bHasThreatTable)
	{
		AddToThreatTable(FThreatTarget(TargetingComponent->GetOwner(), 0.0f, TargetingComponent->HasActiveFade()));
	}
}

void UThreatHandler::OnTargetHealingTaken(FDamagingEvent const& HealingEvent)
{
	if (!HealingEvent.Result.Success || !HealingEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Healing, (HealingEvent.ThreatInfo.BaseThreat * GlobalHealingThreatModifier), HealingEvent.Info.AppliedBy,
		HealingEvent.Info.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
		HealingEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnTargetHealingDone(FDamagingEvent const& HealingEvent)
{
	if (!HealingEvent.Result.Success || !HealingEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	//If a target heals someone not already in our threat table, we won't get a callback from that actor receiving the healing.
	if (!IsActorInThreatTable(HealingEvent.Info.AppliedTo))
	{
		//Add the threat, since we don't get the received healing callback to add it.
		AddThreat(EThreatType::Healing,(HealingEvent.ThreatInfo.BaseThreat * GlobalHealingThreatModifier), HealingEvent.Info.AppliedBy,
			HealingEvent.Info.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
			HealingEvent.ThreatInfo.SourceModifier);
		//Then add the healed target to the threat table as well.
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(HealingEvent.Info.AppliedTo);
		if (IsValid(TargetThreatHandler))
		{
			AddToThreatTable(FThreatTarget(HealingEvent.Info.AppliedTo, 0.0f, TargetThreatHandler->HasActiveFade()));
		}
	}
}

void UThreatHandler::OnTargetLifeStatusChanged(AActor* Actor, ELifeStatus const PreviousStatus,
	ELifeStatus const NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		RemoveFromThreatTable(Actor);
	}
}

void UThreatHandler::RemoveThreat(float const Amount, AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy))
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

void UThreatHandler::RemoveFromThreatTable(AActor* Actor)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Actor))
	{
		return;
	}
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
		{
			bFound = true;
			if (i == ThreatTable.Num() - 1)
			{
				bAffectedTarget = true;
			}
			ThreatTable.RemoveAt(i);
			break;
		}
	}
	if (bFound)
	{
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetingEnded(this);
		}
		UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
		if (IsValid(TargetDamageHandler))
		{
			TargetDamageHandler->UnsubscribeFromOutgoingHealing(ThreatFromOutgoingHealingCallback);
			if (TargetDamageHandler->CanEverReceiveHealing())
			{
				TargetDamageHandler->UnsubscribeFromIncomingHealingSuccess(ThreatFromIncomingHealingCallback);
			}
			if (TargetDamageHandler->HasHealth())
			{
				TargetDamageHandler->UnsubscribeFromLifeStatusChanged(TargetLifeStatusCallback);
			}
		}
		if (bAffectedTarget)
		{
			UpdateTarget();
		}
		if (ThreatTable.Num() == 0)
		{
			UpdateCombat();
		}
	}
}

void UThreatHandler::NotifyOfTargetingEnded(UThreatHandler* TargetingComponent)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(TargetingComponent) || !TargetedBy.Contains(TargetingComponent->GetOwner()))
	{
		return;
	}
	int32 const Removed = TargetedBy.Remove(TargetingComponent->GetOwner());
	if (Removed > 0 && TargetedBy.Num() == 0)
	{
		UpdateCombat();
	}
}

void UThreatHandler::ClearThreatTable()
{
	for (FThreatTarget const& ThreatTarget : ThreatTable)
	{
		UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(ThreatTarget.Target);
		if (IsValid(TargetDamageHandler))
		{
			TargetDamageHandler->UnsubscribeFromOutgoingHealing(ThreatFromOutgoingHealingCallback);
			if (TargetDamageHandler->CanEverReceiveHealing())
			{
				TargetDamageHandler->UnsubscribeFromIncomingHealingSuccess(ThreatFromIncomingHealingCallback);
			}
			if (TargetDamageHandler->HasHealth())
			{
				TargetDamageHandler->UnsubscribeFromLifeStatusChanged(TargetLifeStatusCallback);
			}
		}
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ThreatTarget.Target);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetingEnded(this);
		}
	}
	ThreatTable.Empty();
	UpdateTarget();
	UpdateCombat();
}

bool UThreatHandler::IsActorInThreatTable(AActor* Target) const
{
	if (!bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return false;
	}
	for (FThreatTarget const& ThreatTarget : ThreatTable)
	{
		if (ThreatTarget.Target == Target)
		{
			return true;
		}
	}
	return false;
}

float UThreatHandler::GetActorThreatValue(AActor* Actor) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor))
	{
		return 0.0f;
	}
	for (FThreatTarget const& Target : ThreatTable)
	{
		if (Target.Target == Actor)
		{
			return Target.Threat;
		}
	}
	return 0.0f;
}

void UThreatHandler::GetActorsInThreatTable(TArray<AActor*>& OutActors) const
{
	for (FThreatTarget const& Target : ThreatTable)
	{
		if (IsValid(Target.Target))
		{
			OutActors.Add(Target.Target);
		}
	}
}

void UThreatHandler::UpdateTarget()
{
	AActor* Previous = CurrentTarget;
	if (ThreatTable.Num() == 0)
	{
		CurrentTarget = nullptr;
	}
	else
	{
		CurrentTarget = ThreatTable[ThreatTable.Num() - 1].Target;
	}
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::UpdateCombat()
{
	bool const PreviouslyInCombat = bInCombat;
	bInCombat = ThreatTable.Num() > 0 || TargetedBy.Num() > 0;
	if (bInCombat != PreviouslyInCombat)
	{
		OnCombatChanged.Broadcast(bInCombat);
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(bInCombat);
}

void UThreatHandler::OnRep_CurrentTarget(AActor* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

#pragma endregion 
#pragma region Subscriptions

void UThreatHandler::SubscribeToTargetChanged(FTargetCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnTargetChanged.AddUnique(Callback);
	}
}

void UThreatHandler::UnsubscribeFromTargetChanged(FTargetCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnTargetChanged.Remove(Callback);
	}
}

void UThreatHandler::SubscribeToCombatStatusChanged(FCombatStatusCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCombatChanged.AddUnique(Callback);
	}
}

void UThreatHandler::UnsubscribeFromCombatStatusChanged(FCombatStatusCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCombatChanged.Remove(Callback);
	}
}

#pragma endregion
#pragma region Restrictions

void UThreatHandler::AddIncomingThreatRestriction(UBuff* Source, FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingThreatRestrictions.Add(Source, Restriction);
	}
}

void UThreatHandler::RemoveIncomingThreatRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingThreatRestrictions.Remove(Source);
	}
}

bool UThreatHandler::CheckIncomingThreatRestricted(FThreatEvent const& Event)
{
	for (TTuple<UBuff*, FThreatRestriction> const& Restriction : IncomingThreatRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

void UThreatHandler::AddOutgoingThreatRestriction(UBuff* Source, FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingThreatRestrictions.Add(Source, Restriction);
	}
}

void UThreatHandler::RemoveOutgoingThreatRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingThreatRestrictions.Remove(Source);
	}
}

bool UThreatHandler::CheckOutgoingThreatRestricted(FThreatEvent const& Event)
{
	for (TTuple<UBuff*, FThreatRestriction> const& Restriction : OutgoingThreatRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UThreatHandler::AddIncomingThreatModifier(UBuff* Source, FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		IncomingThreatMods.Add(Source, Modifier);
	}
}

void UThreatHandler::RemoveIncomingThreatModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingThreatMods.Remove(Source);
	}
}

float UThreatHandler::GetModifiedIncomingThreat(FThreatEvent const& ThreatEvent) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FThreatModCondition> const& Modifier : IncomingThreatMods)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(ThreatEvent));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

void UThreatHandler::AddOutgoingThreatModifier(UBuff* Source, FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		OutgoingThreatMods.Add(Source, Modifier);
	}
}

void UThreatHandler::RemoveOutgoingThreatModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingThreatMods.Remove(Source);
	}
}

float UThreatHandler::GetModifiedOutgoingThreat(FThreatEvent const& ThreatEvent, FThreatModCondition const& SourceModifier) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FThreatModCondition> const& Modifier : OutgoingThreatMods)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(ThreatEvent));
		}
	}
	if (SourceModifier.IsBound())
	{
		Mods.Add(SourceModifier.Execute(ThreatEvent));
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

#pragma endregion
#pragma region Threat Special Actions

void UThreatHandler::Taunt(AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy))
	{
		return;
	}
	float const InitialThreat = GetActorThreatValue(AppliedBy);
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
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target))
	{
		return;
	}
	float const DropThreat = GetActorThreatValue(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(DropThreat, Target);
}

void UThreatHandler::TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetActorThreatValue(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, true, true, FThreatModCondition());
}

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	for (AActor* Targeting : TargetedBy)
	{
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetVanished(GetOwner());
		}
	}
}

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target) || !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass())|| !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
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
		AddToThreatTable(FThreatTarget(Target, 0.0f, GeneratorComponent->HasActiveFade(), Source));
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
	
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
		AddToThreatTable(FThreatTarget(Target, 0.0f, GeneratorComponent->HasActiveFade(), nullptr, Source));
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
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
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
		for (AActor* Targeting : TargetedBy)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->NotifyOfTargetFadeStatusUpdate(GetOwner(), true);
			}
		}
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	int32 const Removed = Fades.Remove(Source);
	if (Removed != 0 && Fades.Num() == 0)
	{
		for (AActor* Targeting : TargetedBy)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->NotifyOfTargetFadeStatusUpdate(GetOwner(), false);
			}
		}
	}
}

void UThreatHandler::NotifyOfTargetFadeStatusUpdate(AActor* Target, bool const FadeStatus)
{
	if (!IsValid(Target))
	{
		return;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
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

void UThreatHandler::AddMisdirect(UBuff* Source, AActor* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (FMisdirect const& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			return;
		}
	}
	Misdirects.Add(FMisdirect(Source, Target));
}

void UThreatHandler::RemoveMisdirect(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	for (FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			Misdirects.Remove(Misdirect);
			return;
		}
	}
}

#pragma endregion