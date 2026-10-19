#include "Threat/ThreatHandler.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "AggroRadius.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "CombatGroup.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "CombatStatusComponent.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"

float UThreatHandler::GLOBALHEALINGTHREATMOD = 0.3f;
float UThreatHandler::GLOBALTAUNTTHREATPERCENT = 1.2f;

#pragma region Initialization

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Threat Handler."));
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	checkf(IsValid(CombatStatusComponentRef), TEXT("Owner does not have a valid Combat Status Component, which Threat Handler depends on."));
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
	if (IsValid(AbilityComponent))
	{
		NPCComponentRef = Cast<UNPCAbilityComponent>(AbilityComponent);
	}
	DisableThreatEvents.BindDynamic(this, &UThreatHandler::DisableAllThreatEvents);
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(DamageHandlerRef))
		{
			if (DamageHandlerRef->CanEverReceiveDamage())
			{
				DamageHandlerRef->OnIncomingHealthEvent.AddDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
			}
		}
		if (IsValid(NPCComponentRef))
		{
			NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UThreatHandler::OnCombatBehaviorChanged);
			if (NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::None || NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::Resetting)
			{
				AddOutgoingThreatRestriction(DisableThreatEvents);
				AddIncomingThreatRestriction(DisableThreatEvents);
			}
		}
		if (bProximityAggro && DefaultDetectionRadius > 0.0f)
		{
			AggroRadius = Cast<UAggroRadius>(GetOwner()->AddComponentByClass(UAggroRadius::StaticClass(), false, FTransform::Identity, false));
			if (IsValid(AggroRadius))
			{
				AggroRadius->Initialize(this, DefaultDetectionRadius);
			}
		}
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
	DOREPLIFETIME(UThreatHandler, CombatStartTime);
}

void UThreatHandler::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	const bool bWasDisabled = PreviousBehavior == ENPCCombatBehavior::None || PreviousBehavior == ENPCCombatBehavior::Resetting;
	const bool bShouldBeDisabled = NewBehavior == ENPCCombatBehavior::None || NewBehavior == ENPCCombatBehavior::Resetting;
	if (bWasDisabled && !bShouldBeDisabled)
	{
		RemoveOutgoingThreatRestriction(DisableThreatEvents);
		RemoveIncomingThreatRestriction(DisableThreatEvents);
	}
	else if (!bWasDisabled && bShouldBeDisabled)
	{
		AddOutgoingThreatRestriction(DisableThreatEvents);
		AddIncomingThreatRestriction(DisableThreatEvents);
	}
}

void UThreatHandler::OnOwnerDamageTaken(const FHealthEvent& DamageEvent)
{
	if (!DamageEvent.Result.Success || !DamageEvent.ThreatInfo.bGeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Damage, DamageEvent.ThreatInfo.BaseThreat, DamageEvent.Info.AppliedBy,
		DamageEvent.Info.Source, DamageEvent.ThreatInfo.IgnoreRestrictions, DamageEvent.ThreatInfo.IgnoreModifiers,
		DamageEvent.ThreatInfo.SourceModifier);
}

float UThreatHandler::GetCombatTime() const
{
	if (CombatStartTime <= 0.0f)
	{
		return 0.0f;
	}
	if (!IsValid(GetWorld()) || !IsValid(GetWorld()->GetGameState()))
	{
		return 0.0f;
	}
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - CombatStartTime;
}

#pragma endregion 
#pragma region Threat

void UThreatHandler::NotifyOfNewCombatant(UThreatHandler* Combatant)
{
	const FThreatTarget NewTarget = FThreatTarget(Combatant);
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

void UThreatHandler::NotifyOfCombatantLeft(const UThreatHandler* Combatant)
{
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Combatant)
		{
			ThreatTable.RemoveAt(i);
			if (i == ThreatTable.Num())
			{
				UpdateTarget();
			}
			break;
		}
	}
}

int32 UThreatHandler::FindOrAddToThreatTable(UThreatHandler* Target, bool& bAdded)
{
	bAdded = false;
	if (!IsValid(Target))
	{
		return -1;
	}
	//Check threat table for already existing entry.
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			return i;
		}
	}
	//Not in threat table, need to enter combat with the target.
	if (bInCombat && IsValid(CombatGroup))
	{
		if (Target->IsInCombat() && IsValid(Target->GetCombatGroup()))
		{
			//Both handlers are already in separate combat groups.
			CombatGroup->MergeWith(Target->GetCombatGroup());
		}
		else
		{
			//This handler is in combat but enemy isn't.
			CombatGroup->AddCombatant(Target);
		}
	}
	else
	{
		if (Target->IsInCombat() && IsValid(Target->GetCombatGroup()))
		{
			//This handler is not in combat, but the enemy is.
			Target->GetCombatGroup()->AddCombatant(this);
		}
		else
		{
			//Neither handler is in combat.
			UCombatGroup* NewCombat = NewObject<UCombatGroup>();
			if (!IsValid(NewCombat))
			{
				return -1;
			}
			NewCombat->AddCombatant(this);
			NewCombat->AddCombatant(Target);
		}
	}
	//After entering combat, check the threat table again, as the combat group should've added the new combatant.
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			bAdded = true;
			return i;
		}
	}
	return -1;
}

int32 UThreatHandler::FindInThreatTable(const UThreatHandler* Target) const
{
	if (!IsValid(Target) || !Target->CanBeInThreatTable())
	{
		return -1;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			return i;
		}
	}
	return -1;
}

int32 UThreatHandler::FindInThreatTable(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return -1;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat->GetOwner() == Target)
		{
			return i;
		}
	}
	return -1;
}

void UThreatHandler::ClearThreatTable()
{
	ThreatTable.Empty();
	UpdateTarget();
}

FThreatEvent UThreatHandler::AddThreat(const EThreatType ThreatType, const float BaseThreat, AActor* AppliedBy,
                                       UObject* Source, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const FThreatModCondition& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	if (!bHasThreatTable || BaseThreat <= 0.0f || (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead()))
	{
		return Result;		
	}
	if (!IsValid(AppliedBy) || !AppliedBy->Implements<USaiyoraCombatInterface>())
	{
		return Result;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorThreat) || !GeneratorThreat->CanBeInThreatTable())
	{
		return Result;
	}

	//Make sure the actor applying threat and the one receiving it aren't the same faction.
	const UCombatStatusComponent* GeneratorCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(AppliedBy);
	if (!IsValid(GeneratorCombatStatus) || GeneratorCombatStatus->GetCurrentFaction() == CombatStatusComponentRef->GetCurrentFaction())
	{
		return Result;
	}

	//Check to see if we should redirect this threat to a different actor due to Misdirect.
	AActor* MisdirectTarget = GeneratorThreat->GetMisdirectTarget();
	if (IsValid(MisdirectTarget))
	{
		//GetMisdirectTarget does all the required checks, so if MisdirectTarget is valid we're good to go.
		Result.AppliedBy = MisdirectTarget;
		//No need to IsValid check this, if it wasn't valid then GetMisdirectTarget would've failed.
		const UCombatStatusComponent* MisdirectCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(MisdirectTarget);
		Result.AppliedByPlane = MisdirectCombatStatus->GetCurrentPlane();
	}
	else
	{
		//Make sure the actor generating the threat isn't dead.
		const UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
		if (IsValid(GeneratorHealth) && GeneratorHealth->IsDead())
		{
			return Result;
		}
		Result.AppliedBy = AppliedBy;
		Result.AppliedByPlane = GeneratorCombatStatus->GetCurrentPlane();
	}
	
	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	Result.AppliedToPlane = CombatStatusComponentRef->GetCurrentPlane();
	Result.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		//Note that even if we are misdirecting, we are still using the outgoing threat modifiers from the ACTUAL instigator.
		Result.Threat = GeneratorThreat->GetModifiedOutgoingThreat(Result, SourceModifier);
		Result.Threat = GetModifiedIncomingThreat(Result);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions &&
		//Note that even if we are misdirecting, we are still using the outgoing threat restrictions from the ACTUAL instigator.
		(IncomingThreatRestrictions.IsRestricted(Result) || GeneratorThreat->CheckOutgoingThreatRestricted(Result)))
	{
		return Result;
	}

	//We already know the threat handler is valid, because of the checks above. We re-get it here so that we get the right target factoring in misdirects.
	const int32 TargetIndex = FindOrAddToThreatTable(ISaiyoraCombatInterface::Execute_GetThreatHandler(Result.AppliedBy), Result.bInitialThreat);
	if (TargetIndex == -1)
	{
		//Failed to add to threat table.
		return Result;
	}
	ThreatTable[TargetIndex].Threat += Result.Threat;
	SortModifiedThreatTarget(TargetIndex);
	Result.bSuccess = true;
	return Result;
}

void UThreatHandler::RemoveThreat(const float Amount, const AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy) || Amount <= 0.0f)
	{
		return;
	}
	if (!AppliedBy->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	const UThreatHandler* TargetThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(TargetThreat) || !TargetThreat->CanBeInThreatTable())
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(TargetThreat);
	if (TargetIndex == -1)
	{
		return;
	}
	ThreatTable[TargetIndex].Threat = FMath::Max(0.0f, ThreatTable[TargetIndex].Threat - Amount);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::SortModifiedThreatTarget(const int32 ModifiedIndex)
{
	if (ThreatTable.Num() <= 1 || !ThreatTable.IsValidIndex(ModifiedIndex))
	{
		return;
	}
	int32 CurrentIndex = ModifiedIndex;
	while (CurrentIndex >= 0 && CurrentIndex <= ThreatTable.Num() - 1)
	{
		if (CurrentIndex > 0 && ThreatTable[CurrentIndex] < ThreatTable[CurrentIndex - 1])
		{
			ThreatTable.Swap(CurrentIndex, CurrentIndex - 1);
			CurrentIndex--;
		}
		else if (CurrentIndex < ThreatTable.Num() - 1 && ThreatTable[CurrentIndex + 1] < ThreatTable[CurrentIndex])
		{
			ThreatTable.Swap(CurrentIndex, CurrentIndex + 1);
			CurrentIndex++;
		}
		else
		{
			break;
		}
	}
	if (ModifiedIndex == ThreatTable.Num() - 1 || CurrentIndex == ThreatTable.Num() - 1)
	{
		UpdateTarget();
	}
}

bool UThreatHandler::IsActorInThreatTable(const AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return false;
	}
	return FindInThreatTable(Target) != -1;
}

float UThreatHandler::GetActorThreatValue(const AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return 0.0f;
	}
	const int32 Index = FindInThreatTable(Target);
	if (Index == -1)
	{
		return 0.0f;
	}
	return ThreatTable[Index].Threat;
}

void UThreatHandler::GetActorsInThreatTable(TArray<AActor*>& OutActors) const
{
	OutActors.Empty();
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat)
	{
		return;
	}
	for (const FThreatTarget& Target : ThreatTable)
	{
		if (IsValid(Target.TargetThreat))
		{
			OutActors.Add(Target.TargetThreat->GetOwner());
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
		CurrentTarget = ThreatTable[ThreatTable.Num() - 1].TargetThreat->GetOwner();
	}
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(this, bInCombat);
	NotifyLocalPlayerOfCombatChange();
}

void UThreatHandler::NotifyLocalPlayerOfCombatChange()
{
	if (CombatStatusComponentRef->GetCurrentFaction() == EFaction::Enemy)
	{
		if (!IsValid(LocalPlayer))
		{
			LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(this);
		}
		if (IsValid(LocalPlayer))
		{
			LocalPlayer->NotifyEnemyCombatChanged(GetOwner(), bInCombat);
		}
	}
}

void UThreatHandler::OnRep_CurrentTarget(AActor* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

#pragma endregion 
#pragma region Modifiers

float UThreatHandler::GetModifiedIncomingThreat(const FThreatEvent& ThreatEvent) const
{
	TArray<FCombatModifier> Mods;
	IncomingThreatMods.GetModifiers(Mods, ThreatEvent);
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

float UThreatHandler::GetModifiedOutgoingThreat(const FThreatEvent& ThreatEvent, const FThreatModCondition& SourceModifier) const
{
	TArray<FCombatModifier> Mods;
	OutgoingThreatMods.GetModifiers(Mods, ThreatEvent);
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
	const float InitialThreat = GetActorThreatValue(AppliedBy);
	float HighestThreat = 0.0f;
	for (const FThreatTarget& Target : ThreatTable)
	{
		//Have to search for highest threat because current target might be due to fixate/blind and not highest threat value.
		if (Target.Threat > HighestThreat)
		{
			HighestThreat = Target.Threat;
		}
	}
	HighestThreat *= GLOBALTAUNTTHREATPERCENT;
	AddThreat(EThreatType::Absolute, FMath::Max(0.0f, HighestThreat - InitialThreat), AppliedBy, nullptr, false, true, FThreatModCondition());
}

void UThreatHandler::DropThreat(AActor* Target, const float Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target))
	{
		return;
	}
	const float DropThreat = GetActorThreatValue(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(DropThreat, Target);
}

void UThreatHandler::TransferThreat(AActor* FromActor, AActor* ToActor, const float Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetActorThreatValue(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, false, true, FThreatModCondition());
}

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(CombatGroup))
	{
		return;
	}
	CombatGroup->RemoveCombatant(this);
}

#pragma endregion
#pragma region Fixate

void UThreatHandler::AddFixate(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!CanFixate(GetOwner(), Target))
	{
		return;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	bool bAdded = false;
	const int32 TargetIndex = FindOrAddToThreatTable(GeneratorThreat, bAdded);
	if (TargetIndex == -1)
	{
		//Failed to add to threat table.
		return;
	}
	if (ThreatTable[TargetIndex].Fixates.Contains(Source))
	{
		return;
	}
	ThreatTable[TargetIndex].Fixates.Add(Source);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::RemoveFixate(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(Target);
	if (TargetIndex == -1)
	{
		return;
	}
	const int32 Removed = ThreatTable[TargetIndex].Fixates.Remove(Source);
	if (Removed > 0)
	{
		SortModifiedThreatTarget(TargetIndex);
	}
}

bool UThreatHandler::CanFixate(const AActor* Fixating, const AActor* FixatedOn)
{
	//Validity/interface checks
	if (!IsValid(Fixating) || !Fixating->Implements<USaiyoraCombatInterface>()
		|| !IsValid(FixatedOn) || !FixatedOn->Implements<USaiyoraCombatInterface>())
	{
		return false;
	}
	//Check that the fixating actor has a threat handler and a threat table.
	const UThreatHandler* FixatingThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Fixating);
	if (!IsValid(FixatingThreatHandler) || !FixatingThreatHandler->HasThreatTable())
	{
		return false;
	}
	//Check that the fixate target has a threat handler and can be in a threat table.
	const UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(FixatedOn);
	if (!IsValid(TargetThreatHandler) || !TargetThreatHandler->CanBeInThreatTable())
	{
		return false;
	}
	//Check that both actors have combat status components and aren't the same faction.
	const UCombatStatusComponent* FixatingStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Fixating);
	if (!IsValid(FixatingStatusComponent))
	{
		return false;
	}
	const UCombatStatusComponent* TargetStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(FixatedOn);
	if (!IsValid(TargetStatusComponent))
	{
		return false;
	}
	if (FixatingStatusComponent->GetCurrentFaction() == TargetStatusComponent->GetCurrentFaction())
	{
		return false;
	}
	//Make sure both parties aren't dead.
	const UDamageHandler* FixatingDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Fixating);
	if (IsValid(FixatingDamageHandler) && FixatingDamageHandler->IsDead())
	{
		return false;
	}
	const UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(FixatedOn);
	if (IsValid(TargetDamageHandler) && TargetDamageHandler->IsDead())
	{
		return false;
	}
	return true;
}

#pragma endregion
#pragma region Blind

void UThreatHandler::AddBlind(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!CanBlind(GetOwner(), Target))
	{
		return;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	bool bAdded = false;
	const int32 TargetIndex = FindOrAddToThreatTable(GeneratorThreat, bAdded);
	if (TargetIndex == -1)
	{
		return;
	}
	if (ThreatTable[TargetIndex].Blinds.Contains(Source))
	{
		return;
	}
	ThreatTable[TargetIndex].Blinds.Add(Source);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::RemoveBlind(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(Target);
	if (TargetIndex == -1)
	{
		return;
	}
	const int32 Removed = ThreatTable[TargetIndex].Blinds.Remove(Source);
	if (Removed > 0)
	{
		SortModifiedThreatTarget(TargetIndex);
	}
}

bool UThreatHandler::CanBlind(const AActor* Blinded, const AActor* BlindTo)
{
	//Validity/interface checks
	if (!IsValid(Blinded) || !Blinded->Implements<USaiyoraCombatInterface>()
		|| !IsValid(BlindTo) || !BlindTo->Implements<USaiyoraCombatInterface>())
	{
		return false;
	}
	//Check that the blinded actor has a threat handler and a threat table.
	const UThreatHandler* BlindedThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Blinded);
	if (!IsValid(BlindedThreatHandler) || !BlindedThreatHandler->HasThreatTable())
	{
		return false;
	}
	//Check that the actor to blind the target to has a threat handler and can be in a threat table.
	const UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(BlindTo);
	if (!IsValid(TargetThreatHandler) || !TargetThreatHandler->CanBeInThreatTable())
	{
		return false;
	}
	//Check that both actors have combat status components and aren't the same faction.
	const UCombatStatusComponent* BlindedStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Blinded);
	if (!IsValid(BlindedStatusComponent))
	{
		return false;
	}
	const UCombatStatusComponent* TargetStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(BlindTo);
	if (!IsValid(TargetStatusComponent))
	{
		return false;
	}
	if (BlindedStatusComponent->GetCurrentFaction() == TargetStatusComponent->GetCurrentFaction())
	{
		return false;
	}
	//Make sure both parties aren't dead.
	const UDamageHandler* BlindedDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Blinded);
	if (IsValid(BlindedDamageHandler) && BlindedDamageHandler->IsDead())
	{
		return false;
	}
	const UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(BlindTo);
	if (IsValid(TargetDamageHandler) && TargetDamageHandler->IsDead())
	{
		return false;
	}
	return true;
}

#pragma endregion
#pragma region Fade

void UThreatHandler::AddFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
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
		if (IsValid(CombatGroup))
		{
			CombatGroup->UpdateCombatantFadeStatus(this, true);
		}
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	const int32 Removed = Fades.Remove(Source);
	if (Removed > 0 && Fades.Num() == 0)
	{
		if (IsValid(CombatGroup))
		{
			CombatGroup->UpdateCombatantFadeStatus(this, false);
		}
	}
}

void UThreatHandler::NotifyOfCombatantFadeStatus(const UThreatHandler* Combatant, const bool FadeStatus)
{
	const int32 TargetIndex = FindInThreatTable(Combatant);
	if (TargetIndex == -1)
	{
		return;
	}
	ThreatTable[TargetIndex].Faded = FadeStatus;
	SortModifiedThreatTarget(TargetIndex);
}

#pragma endregion
#pragma region Misdirect

void UThreatHandler::AddMisdirect(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!CanMisdirectThreat(GetOwner(), Target))
	{
		return;
	}
	for (const FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			return;
		}
	}
	Misdirects.Add(FMisdirect(Source, Target));
}

void UThreatHandler::RemoveMisdirect(const UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	for (const FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			Misdirects.Remove(Misdirect);
			return;
		}
	}
}

AActor* UThreatHandler::GetMisdirectTarget() const
{
	for (int i = Misdirects.Num() - 1; i >= 0; i--)
	{
		if (CanMisdirectThreat(GetOwner(), Misdirects[i].Target))
		{
			return Misdirects[i].Target;
		}
	}
	return nullptr;
}

bool UThreatHandler::CanMisdirectThreat(const AActor* ThreatFrom, const AActor* ThreatTo)
{
	//Validity/interface checks and also making sure From and To aren't the same actor.
	if (!IsValid(ThreatFrom) || !ThreatFrom->Implements<USaiyoraCombatInterface>()
		|| !IsValid(ThreatTo) || !ThreatTo->Implements<USaiyoraCombatInterface>()
		|| ThreatFrom == ThreatTo)
	{
		return false;
	}
	//Check that both From and To have threat handlers and can be in threat tables.
	const UThreatHandler* FromThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ThreatFrom);
	if (!IsValid(FromThreatHandler) || !FromThreatHandler->CanBeInThreatTable())
	{
		return false;
	}
	const UThreatHandler* ToThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ThreatTo);
	if (!IsValid(ToThreatHandler) || !ToThreatHandler->CanBeInThreatTable())
	{
		return false;
	}
	//Check that both From and To have combat status components and are the same faction.
	const UCombatStatusComponent* FromStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(ThreatFrom);
	if (!IsValid(FromStatusComponent))
	{
		return false;
	}
	const UCombatStatusComponent* ToStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(ThreatTo);
	if (!IsValid(ToStatusComponent))
	{
		return false;
	}
	if (FromStatusComponent->GetCurrentFaction() != ToStatusComponent->GetCurrentFaction())
	{
		return false;
	}
	//If the To target has a damage handler, make sure he's not dead. We actually let the From actor be dead (DoTs can generate threat while dead, for example).
	const UDamageHandler* ToDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(ThreatTo);
	if (IsValid(ToDamageHandler) && ToDamageHandler->IsDead())
	{
		return false;
	}
	return true;
}

#pragma endregion
#pragma region Combat Group

void UThreatHandler::NotifyOfCombat(UCombatGroup* Group)
{
	const bool bPreviouslyInCombat = bInCombat;
	if (!IsValid(Group))
	{
		if (bPreviouslyInCombat)
		{
			CombatGroup = nullptr;
			bInCombat = false;
			CombatStartTime = 0.0f;
			ClearThreatTable();
			OnCombatChanged.Broadcast(this, false);
			NotifyLocalPlayerOfCombatChange();
		}
		return;
	}
	CombatGroup = Group;
	if (!bPreviouslyInCombat)
	{
		bInCombat = true;
		CombatStartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		OnCombatChanged.Broadcast(this, true);
		NotifyLocalPlayerOfCombatChange();
	}
}

#pragma endregion
