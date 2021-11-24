#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatGroup.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraBuffLibrary.h"
#include "PlaneComponent.h"
#include "Faction/FactionComponent.h"

float UThreatHandler::GlobalHealingThreatModifier = 0.3f;
float UThreatHandler::GlobalTauntThreatPercentage = 1.2f;
float UThreatHandler::GlobalThreatDecayPercentage = 0.9f;
float UThreatHandler::GlobalThreatDecayInterval = 3.0f;

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
		FadeCallback.BindDynamic(this, &UThreatHandler::OnTargetFadeStatusChanged);
		ThreatFromDamageCallback.BindDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
		ThreatFromHealingCallback.BindDynamic(this, &UThreatHandler::OnTargetHealingTaken);
		ThreatBuffRestriction.BindDynamic(this, &UThreatHandler::CheckBuffForThreat);
		DecayDelegate.BindUObject(this, &UThreatHandler::DecayThreat);
		CombatantAddedCallback.BindDynamic(this, &UThreatHandler::AddNewCombatantToThreatTable);
		CombatantRemovedCallback.BindDynamic(this, &UThreatHandler::RemoveCombatantFromThreatTable);
	}
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		if (IsValid(BuffHandlerRef))
		{
			if (!bCanBeInThreatTable)
			{
				BuffHandlerRef->AddOutgoingBuffRestriction(ThreatBuffRestriction);
			}
			if (!bHasThreatTable)
			{
				BuffHandlerRef->AddIncomingBuffRestriction(ThreatBuffRestriction);
			}
		}
		DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->SubscribeToIncomingDamageSuccess(ThreatFromDamageCallback);
		}
		PlaneCompRef = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
		FactionCompRef = ISaiyoraCombatInterface::Execute_GetFactionComponent(GetOwner());
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

#pragma region Combat

void UThreatHandler::StartNewCombat(UThreatHandler* TargetThreatHandler)
{
	CombatGroup = NewObject<UCombatGroup>(GetOwner());
	if (!IsValid(CombatGroup))
	{
		CombatGroup = nullptr;	
	}
	CombatGroup->JoinCombat(GetOwner());
	CombatGroup->JoinCombat(TargetThreatHandler->GetOwner());
}

void UThreatHandler::AddNewCombatantToThreatTable(UCombatGroup* Group, AActor* Combatant)
{
	if (Group == CombatGroup && Combatant != GetOwner())
	{
		AddToThreatTable(Combatant);
	}
}

void UThreatHandler::RemoveCombatantFromThreatTable(UCombatGroup* Group, AActor* Combatant)
{
	if (Group == CombatGroup && Combatant != GetOwner())
	{
		RemoveFromThreatTable(Combatant);
	}
}

void UThreatHandler::NotifyOfCombatJoined(UCombatGroup* NewGroup)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(NewGroup))
	{
		return;
	}
	CombatGroup = NewGroup;
	if (!bInCombat)
	{
		bInCombat = true;
		OnCombatChanged.Broadcast(true);
	}
	TArray<AActor*> Combatants;
	NewGroup->GetActorsInCombat(Combatants);
	for (AActor* Combatant : Combatants)
	{
		if (Combatant != GetOwner())
		{
			AddToThreatTable(Combatant);
		}
	}
	CombatGroup->SubscribeToCombatantAdded(CombatantAddedCallback);
	CombatGroup->SubscribeToCombatantRemoved(CombatantRemovedCallback);
}

void UThreatHandler::NotifyOfCombatLeft(UCombatGroup* LeftGroup)
{
	if (GetOwnerRole() != ROLE_Authority || !bInCombat || !IsValid(LeftGroup) || LeftGroup != CombatGroup)
	{
		return;
	}
	CombatGroup->UnsubscribeFromCombatantAdded(CombatantAddedCallback);
	CombatGroup->UnsubscribeFromCombatantRemoved(CombatantRemovedCallback);
	CombatGroup = nullptr;
	bInCombat = false;
	OnCombatChanged.Broadcast(false);
	ClearThreatTable();
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(bInCombat);
}

#pragma endregion
#pragma region Threat

FThreatEvent UThreatHandler::AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
                                       UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || BaseThreat <= 0.0f || !bHasThreatTable || !IsValid(FactionCompRef))
	{
		return Result;
	}
	
	//Target must either be alive, or not have a health component.
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead())
	{
		return Result;
	}
	
	if (!AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return Result;
	}
	//Generator must either be alive, or not have a health component.
	UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->IsDead())
	{
		return Result;
	}
	//Generator must have a faction, and can not be the same faction as the target.
	UFactionComponent* GeneratorFaction = ISaiyoraCombatInterface::Execute_GetFactionComponent(AppliedBy);
	if (!IsValid(GeneratorFaction) || GeneratorFaction->GetCurrentFaction() == FactionCompRef->GetCurrentFaction())
	{
		return Result;
	}
	//TODO: Check misdirect target faction?
	if (IsValid(GeneratorComponent->GetMisdirectTarget()) && GeneratorComponent->GetMisdirectTarget()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UThreatHandler* MisdirectComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(GeneratorComponent->GetMisdirectTarget());
		if (IsValid(MisdirectComponent) && MisdirectComponent->CanBeInThreatTable())
		{
			UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorComponent->GetMisdirectTarget());
			if (IsValid(MisdirectHealth) && MisdirectHealth->IsDead())
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
	}
	else
	{
		Result.AppliedBy = AppliedBy;
	}

	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(AppliedBy);
	Result.AppliedByPlane = IsValid(AppliedByPlaneComp) ? AppliedByPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedToPlane = IsValid(PlaneCompRef) ? PlaneCompRef->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedXPlane = UPlaneComponent::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		Result.Threat = GeneratorComponent->GetModifiedOutgoingThreat(Result, SourceModifier);
		Result.Threat = GetModifiedIncomingThreat(Result);
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

void UThreatHandler::AddToThreatTable(AActor* Actor, float const InitialThreat, UBuff* InitialFixate,
                                      UBuff* InitialBlind)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority || !bHasThreatTable || IsActorInThreatTable(Actor) || !IsValid(FactionCompRef))
	{
		return;
	}
	UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
	if (!IsValid(TargetThreatHandler) || !TargetThreatHandler->CanBeInThreatTable())
	{
		return;
	}
	UFactionComponent* TargetFactionComp = ISaiyoraCombatInterface::Execute_GetFactionComponent(Actor);
	if (!IsValid(TargetFactionComp) || TargetFactionComp->GetCurrentFaction() == FactionCompRef->GetCurrentFaction())
	{
		return;
	}
	bool const TargetFade = TargetThreatHandler->HasActiveFade();
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Actor, InitialThreat, TargetFade, InitialFixate, InitialBlind));
		UpdateTarget();
	}
	else
	{
		FThreatTarget const NewTarget = FThreatTarget(Actor, InitialThreat, TargetFade, InitialFixate, InitialBlind);
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
	TargetThreatHandler->SubscribeToFadeStatusChanged(FadeCallback);
	UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
	if (IsValid(TargetDamageHandler) && TargetDamageHandler->CanEverReceiveHealing())
	{
		TargetDamageHandler->SubscribeToIncomingHealingSuccess(ThreatFromHealingCallback);
	}
	if (TargetThreatHandler->IsInCombat() && IsValid(TargetThreatHandler->GetCombatGroup()))
	{
		if (bInCombat && IsValid(CombatGroup))
		{
			if (CombatGroup != TargetThreatHandler->GetCombatGroup())
			{
				CombatGroup->MergeGroups(TargetThreatHandler->GetCombatGroup());
			}
		}
		else
		{
			TargetThreatHandler->GetCombatGroup()->JoinCombat(GetOwner());
		}
	}
	else
	{
		if (bInCombat && IsValid(CombatGroup))
		{
			CombatGroup->JoinCombat(TargetThreatHandler->GetOwner());
		}
		else
		{
			StartNewCombat(TargetThreatHandler);
		}
	}
}

void UThreatHandler::RemoveThreat(float const Amount, AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bHasThreatTable)
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
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority || !bHasThreatTable)
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
			}
			UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
			if (IsValid(TargetDamageHandler) && TargetDamageHandler->CanEverReceiveHealing())
			{	
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
}

void UThreatHandler::ClearThreatTable()
{
	//TODO: Clear Threat Table.
}

void UThreatHandler::DecayThreat()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (FThreatTarget& Target : ThreatTable)
		{
			Target.Threat *= GlobalThreatDecayPercentage;
		}
	}
}

bool UThreatHandler::IsActorInThreatTable(AActor* Target) const
{
	if (!IsValid(Target) || !bHasThreatTable || !bInCombat)
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

#pragma endregion
#pragma region Restrictions

void UThreatHandler::AddIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingThreatRestrictions.AddUnique(Restriction);
	}
}

void UThreatHandler::RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingThreatRestrictions.Remove(Restriction);
	}
}

bool UThreatHandler::CheckIncomingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatRestriction const& Restriction : IncomingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

void UThreatHandler::AddOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingThreatRestrictions.AddUnique(Restriction);
	}
}

void UThreatHandler::RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingThreatRestrictions.Remove(Restriction);
	}
}

bool UThreatHandler::CheckOutgoingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatRestriction const& Restriction : OutgoingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UThreatHandler::AddIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingThreatMods.AddUnique(Modifier);
	}
}

void UThreatHandler::RemoveIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingThreatMods.Remove(Modifier);
	}
}

float UThreatHandler::GetModifiedIncomingThreat(FThreatEvent const& ThreatEvent) const
{
	TArray<FCombatModifier> Mods;
	for (FThreatModCondition const& Modifier : IncomingThreatMods)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(ThreatEvent));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

void UThreatHandler::AddOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingThreatMods.AddUnique(Modifier);
	}
}

void UThreatHandler::RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingThreatMods.Remove(Modifier);
	}
}

float UThreatHandler::GetModifiedOutgoingThreat(FThreatEvent const& ThreatEvent, FThreatModCondition const& SourceModifier) const
{
	TArray<FCombatModifier> Mods;
	for (FThreatModCondition const& Modifier : OutgoingThreatMods)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(ThreatEvent));
		}
	}
	if (SourceModifier.IsBound())
	{
		Mods.Add(SourceModifier.Execute(ThreatEvent));
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

#pragma endregion 

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

void UThreatHandler::AddMisdirect(UBuff* Source, AActor* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target))
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

void UThreatHandler::Taunt(AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bHasThreatTable)
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !bHasThreatTable)
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

bool UThreatHandler::CheckBuffForThreat(FBuffApplyEvent const& BuffEvent)
{
	if (!IsValid(BuffEvent.BuffClass))
	{
		return false;
	}
	UBuff* DefaultBuff = BuffEvent.BuffClass.GetDefaultObject();
	if (!IsValid(DefaultBuff))
	{
		return false;
	}
	FGameplayTagContainer BuffTags;	
	DefaultBuff->GetBuffTags(BuffTags);
	return BuffTags.HasTag(GenericThreatTag());
}

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
	UDamageHandler* HealthComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(Target);
	if (IsValid(HealthComponent) && HealthComponent->IsDead())
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
		AddToThreatTable(Target, 0.0f, Source);
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
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
	UDamageHandler* HealthComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(Target);
	if (IsValid(HealthComponent) && HealthComponent->IsDead())
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
		AddToThreatTable(Target, 0.0f, nullptr, Source);
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
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Source))
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
	//TODO: Vanish.
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