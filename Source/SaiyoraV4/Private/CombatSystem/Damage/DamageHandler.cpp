#include "DamageHandler.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatDebugOptions.h"
#include "StatHandler.h"
#include "CombatStatusComponent.h"
#include "DamageBuffFunctions.h"
#include "DeathOverlay.h"
#include "SaiyoraCombatInterface.h"
#include "DungeonGameState.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraDevSettings.h"
#include "UnrealNetwork.h"

#pragma region Core

UDamageHandler::UDamageHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UDamageHandler::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Damage Handler."));
	if (GetOwnerRole() == ROLE_Authority)
	{
		CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
		StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
		if (bHasHealth)
		{
			MaxHealth = DefaultMaxHealth;
			CurrentHealth = MaxHealth;
		}
		LifeStatus = ELifeStatus::Invalid;
		MaxHealthStatCallback.BindDynamic(this, &UDamageHandler::ReactToMaxHealthStat);
		UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
		if (IsValid(AbilityComponent))
		{
			NPCComponentRef = Cast<UNPCAbilityComponent>(AbilityComponent);
		}
		//NPCs that can respawn should auto accept any resurrections they receive.
		//This allows for mechanics like bosses that resurrect their adds.
		if (bCanBeResurrected && !GetOwner()->GetClass()->IsChildOf(ASaiyoraPlayerCharacter::StaticClass()))
		{
			OnPendingResListChanged.AddDynamic(this, &UDamageHandler::QueueResurrectionAutoAccept);
		}
	}
}

void UDamageHandler::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerAsPawn = Cast<APawn>(GetOwner());
	GameStateRef = GetWorld()->GetGameState<ADungeonGameState>();
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Set respawn point to initial actor location on BeginPlay.
		UpdateRespawnPoint(GetOwner()->GetActorLocation());
		if (IsValid(StatHandlerRef))
		{
			if (bHasHealth && !bStaticMaxHealth && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_MaxHealth))
			{
				UpdateMaxHealth(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_MaxHealth));
				StatHandlerRef->SubscribeToStatChanged(FSaiyoraCombatTags::Get().Stat_MaxHealth, MaxHealthStatCallback);
			}
		}
		if (IsValid(NPCComponentRef))
		{
			NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UDamageHandler::OnCombatBehaviorChanged);
		}
		LifeStatus = ELifeStatus::Alive;
		OnLifeStatusChanged.Broadcast(GetOwner(), ELifeStatus::Invalid, LifeStatus);
	}
}

void UDamageHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDamageHandler, CurrentHealth);
	DOREPLIFETIME(UDamageHandler, MaxHealth);
	DOREPLIFETIME(UDamageHandler, CurrentAbsorb);
	DOREPLIFETIME(UDamageHandler, LifeStatus);
	DOREPLIFETIME(UDamageHandler, PendingResurrections);
}

void UDamageHandler::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	//TODO: This should be a call from the NPCBehaviorComponent itself, probably.
	//I don't think the damage handler should be in charge of healing on reset.
	if (USaiyoraCombatLibrary::IsActiveNPCBehavior(PreviousBehavior)
		&& !USaiyoraCombatLibrary::IsActiveNPCBehavior(NewBehavior))
	{
		ApplyHealthEvent(EHealthEventType::Healing, MaxHealth, GetOwner(), this, EEventHitStyle::Authority, EElementalSchool::None, true,
			true, true, true, FHealthEventModCondition(), FThreatFromDamage());
	}
}

#pragma endregion 
#pragma region Health

void UDamageHandler::UpdateMaxHealth(const float NewMaxHealth)
{
	if (!bHasHealth)
	{
		return;
	}
	const float PreviousMaxHealth = MaxHealth;
	MaxHealth = bStaticMaxHealth ? FMath::Max(1.0f, DefaultMaxHealth) : FMath::Max(1.0f, NewMaxHealth);
	if (MaxHealth != PreviousMaxHealth)
	{
		const float PreviousHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp((CurrentHealth / PreviousMaxHealth) * MaxHealth, 1.0f, MaxHealth);
		if (CurrentHealth != PreviousHealth)
		{
			OnHealthChanged.Broadcast(GetOwner(), PreviousHealth, CurrentHealth);
		}
		const float PreviousAbsorb = CurrentAbsorb;
		CurrentAbsorb = FMath::Clamp(CurrentAbsorb, 0.0f, MaxHealth);
		if (CurrentAbsorb != PreviousAbsorb)
		{
			OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
		}
		OnMaxHealthChanged.Broadcast(GetOwner(), PreviousMaxHealth, MaxHealth);
	}
}

void UDamageHandler::ReactToMaxHealthStat(const FGameplayTag StatTag, const float NewValue)
{
	if (StatTag.IsValid() && StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat_MaxHealth))
	{
		UpdateMaxHealth(NewValue);
	}
}

void UDamageHandler::OnRep_CurrentHealth(const float PreviousValue)
{
	OnHealthChanged.Broadcast(GetOwner(), PreviousValue, CurrentHealth);
}

void UDamageHandler::OnRep_MaxHealth(const float PreviousValue)
{
	OnMaxHealthChanged.Broadcast(GetOwner(), PreviousValue, MaxHealth);
}

void UDamageHandler::OnRep_CurrentAbsorb(const float PreviousValue)
{
	OnAbsorbChanged.Broadcast(GetOwner(), PreviousValue, CurrentAbsorb);	
}

#pragma endregion
#pragma region Death and Resurrection

void UDamageHandler::KillActor(AActor* Attacker, UObject* Source, const bool bIgnoreDeathRestrictions)
{
	ApplyHealthEvent(EHealthEventType::Damage, CurrentHealth, Attacker, Source, EEventHitStyle::Authority, EElementalSchool::None, true, true,
	                 true, bIgnoreDeathRestrictions, FHealthEventModCondition(), FThreatFromDamage());
}

void UDamageHandler::Server_RequestRespawn_Implementation()
{
	const bool bRespawned = RespawnActor();
	//If this is a player, let him know explicitly that we failed to respawn.
	//This unlocks the accept/decline/respawn buttons on the UI.
	if (!bRespawned && IsValid(Cast<ASaiyoraPlayerCharacter>(GetOwner())))
	{
		Client_ResurrectionRequestFailed();
	}
}

void UDamageHandler::Die()
{
	//Clear the pending killing blow, if we have one.
	if (bHasPendingKillingBlow)
	{
		bHasPendingKillingBlow = false;
		PendingKillingBlow = FHealthEvent();
	}
	//Reset absorb and health to 0.
	if (CurrentAbsorb != 0.0f)
	{
		const float PreviousAbsorb = CurrentAbsorb;
		CurrentAbsorb = 0.0f;
		OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
	}
	if (CurrentHealth != 0.0f)
	{
		const float PreviousHealth = CurrentHealth;
		CurrentHealth = 0.0f;
		OnHealthChanged.Broadcast(GetOwner(), PreviousHealth, CurrentHealth);
	}
	//Update life status.
	const ELifeStatus PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Dead;
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
	//Alert the gamestate of our death to add kill count, mark a dead boss, or add to the death penalty timer.
	if (GameStateRef)
	{
		switch (KillCountType)
		{
		case EKillCountType::Player :
			GameStateRef->ReportPlayerDeath();
			break;
		case EKillCountType::Trash :
			GameStateRef->ReportTrashDeath(KillCountValue);
			break;
		case EKillCountType::Boss :
			GameStateRef->ReportBossDeath(BossTag);
			break;
		default :
			break;
		}
	}
}

bool UDamageHandler::RespawnActor(const bool bForceRespawnLocation, const FVector& OverrideRespawnLocation,
                                  const bool bForceHealthPercentage, const float OverrideHealthPercentage)
{
	return RespawnActor_Internal(bForceRespawnLocation, OverrideRespawnLocation, bForceHealthPercentage, OverrideHealthPercentage);
}

bool UDamageHandler::RespawnActor(const FPendingResurrection& Resurrection)
{
	return RespawnActor_Internal(true, Resurrection.ResLocation, Resurrection.bOverrideHealth, Resurrection.OverrideHealthPercent, Resurrection);
}

bool UDamageHandler::RespawnActor_Internal(const bool bForceRespawnLocation, const FVector& OverrideRespawnLocation, const bool bForceHealthPercentage, const float OverrideHealthPercentage,
                                           const FPendingResurrection& Resurrection)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}
	if (!bCanBeResurrected)
	{
		DEBUG_LOG_RESPAWN_FAILED(this, Resurrection, TEXT("Combatant can never respawn."));
		return false;
	}
	if (LifeStatus != ELifeStatus::Dead)
	{
		DEBUG_LOG_RESPAWN_FAILED(this, Resurrection, TEXT("Combatant was not dead."));
		return false;
	}
	if (!bRespawnInPlace || bForceRespawnLocation)
	{
		GetOwner()->SetActorLocation(bForceRespawnLocation ? OverrideRespawnLocation : RespawnLocation);
	}
	const ELifeStatus PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Alive;
	CurrentHealth = FMath::Clamp(MaxHealth * (bForceHealthPercentage ? FMath::Clamp(OverrideHealthPercentage, 0.0f, 1.0f) : 1.0f), 1.0f, MaxHealth);
	const float PreviousAbsorb = CurrentAbsorb;
	CurrentAbsorb = 0.0f;
	
	DEBUG_LOG_RESPAWN(this, Resurrection);

	if (PreviousAbsorb != CurrentAbsorb)
	{
		OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
	}
	OnHealthChanged.Broadcast(GetOwner(), 0.0f, CurrentHealth);
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, ELifeStatus::Alive);
	return true;
}

void UDamageHandler::UpdateRespawnPoint(const FVector& NewLocation)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeResurrected || bRespawnInPlace)
	{
		return;
	}
	RespawnLocation = NewLocation;
}

bool UDamageHandler::AddPendingResurrection(UPendingResurrectionFunction* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !bCanBeResurrected)
	{
		return false;
	}
	for (const FPendingResurrection& PendingRes : PendingResurrections)
	{
		if (PendingRes.Source == Source)
		{
			return false;
		}
	}
	
	const FPendingResurrection& Resurrection = PendingResurrections.Emplace_GetRef(Source);
	DEBUG_LOG_RESURRECTION_OFFERED(this, Resurrection);
	OnPendingResListChanged.Broadcast(PendingResurrections);
	return true;
}

void UDamageHandler::RemovePendingResurrection(UPendingResurrectionFunction* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeResurrected || !IsValid(Source))
	{
		return;
	}
	for (int i = PendingResurrections.Num() - 1; i >= 0; i--)
	{
		if (PendingResurrections[i].Source == Source)
		{
			DEBUG_LOG_RESURRECTION_REMOVED(this, PendingResurrections[i]);
			PendingResurrections.RemoveAt(i);
			OnPendingResListChanged.Broadcast(PendingResurrections);
			break;
		}
	}
}

void UDamageHandler::MakeResurrectionDecision(const int ResID, const bool bAccept)
{
	//Local player needs to RPC to the server to make a res decision.
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_MakeResDecision(ResID, bAccept);
		return;
	}
	//If we're not the server, don't do anything.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	bool bRespawned = false;
	//If we're declining a res, we can do that regardless of whether that res is actually viable.
	//Otherwise, if we're accepting a res, we need to actually be resurrectable and dead.
	if (ResID > -1 && (!bAccept || (bCanBeResurrected && LifeStatus == ELifeStatus::Dead)))
	{
		for (int i = 0; i < PendingResurrections.Num(); i++)
		{
			const FPendingResurrection& PendingRes = PendingResurrections[i];
			if (PendingRes.ResID == ResID)
			{
				const FPendingResurrection ResCopy = PendingResurrections[i];
				PendingResurrections.RemoveAt(i);
				DEBUG_LOG_RESURRECTION_DECISION(this, ResCopy, bAccept);
				//Play cosmetic effects tied to accepting/declining the res.
				if (IsValid(ResCopy.Source))
				{
					ResCopy.Source->OnResDecisionMade(bAccept);
				}
				//If we're accepting, then we just respawn the actor. This will automatically remove all pending resurrection functions.
				if (bAccept)
				{
					bRespawned = RespawnActor(ResCopy);
				}
				//If we're declining, just remove the buff manually.
				else if (IsValid(ResCopy.Source))
				{
					ResCopy.Source->GetOwningBuff()->GetHandler()->RemoveBuff(ResCopy.Source->GetOwningBuff(), EBuffExpireReason::Absolute);
				}
				OnPendingResListChanged.Broadcast(PendingResurrections);
				break;
			}
		}
	}
	//If this is a player, let him know explicitly if we failed to accept this resurrection.
	//This unlocks the accept/decline/respawn buttons on the UI.
	if (bAccept && !bRespawned && IsValid(Cast<ASaiyoraPlayerCharacter>(GetOwner())))
	{
		Client_ResurrectionRequestFailed();
	}
}

void UDamageHandler::Client_ResurrectionRequestFailed_Implementation()
{
	if (const ASaiyoraPlayerCharacter* PlayerOwner = Cast<ASaiyoraPlayerCharacter>(GetOwner()))
	{
		if (IsValid(PlayerOwner->GetDeathOverlay()))
		{
			PlayerOwner->GetDeathOverlay()->OnServerFailedToRespawn();
		}
	}
}

void UDamageHandler::QueueResurrectionAutoAccept(const TArray<FPendingResurrection>& NewResurrections)
{
	if (PendingResurrections.Num() == 0)
	{
		return;
	}
	const FTimerDelegate AutoAcceptDelegate = FTimerDelegate::CreateUObject(this, &UDamageHandler::AutoAcceptResurrection, PendingResurrections[0].ResID);
	GetWorld()->GetTimerManager().SetTimerForNextTick(AutoAcceptDelegate);
}

void UDamageHandler::AutoAcceptResurrection(const int ResID)
{
	if (GetLifeStatus() == ELifeStatus::Dead)
	{
		MakeResurrectionDecision(ResID, true);
	}
}

#pragma endregion 
#pragma region Damage

FHealthEvent UDamageHandler::ApplyHealthEvent_Internal(const EHealthEventType EventType, const float Amount, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
	const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions, const bool bFromSnapshot,
	const FHealthEventSnapshot& Snapshot, const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams)
{
	FHealthEvent HealthEvent;
	HealthEvent.Info.EventID = USaiyoraCombatLibrary::GetUniqueCombatEventID();
	HealthEvent.Info.EventType = EventType;
	HealthEvent.Info.Value = Amount;
	if (bFromSnapshot)
	{
		HealthEvent.Result.Snapshot = Snapshot;
	}
	else
	{
		HealthEvent.Result.Snapshot.Value = Amount;
	}
	HealthEvent.Info.AppliedBy = AppliedBy;
	HealthEvent.Info.AppliedTo = GetOwner();
	HealthEvent.Info.Source = Source;
	HealthEvent.Info.HitStyle = HitStyle;
	HealthEvent.Info.School = School;
	if (IsValid(AppliedBy) && AppliedBy->Implements<USaiyoraCombatInterface>())
	{
		const UCombatStatusComponent* AppliedByCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(AppliedBy);
		HealthEvent.Info.AppliedByPlane = IsValid(AppliedByCombatStatusComp) ? AppliedByCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
	}
	else
	{
		HealthEvent.Info.AppliedByPlane = ESaiyoraPlane::Both;
	}
	HealthEvent.Info.AppliedToPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
	HealthEvent.Info.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(HealthEvent.Info.AppliedByPlane, HealthEvent.Info.AppliedToPlane);
	if (ThreatParams.bGeneratesThreat)
	{
		HealthEvent.ThreatInfo = ThreatParams;
		if (!ThreatParams.bSeparateBaseThreat)
		{
			HealthEvent.ThreatInfo.BaseThreat = HealthEvent.Info.Value;
		}
	}
	//Generic checks for role, validity, blanket "invalid event" type restrictions.
    if (GetOwnerRole() != ROLE_Authority)
    {
    	HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::NetRole);
    }
	if (!IsValid(AppliedBy))
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::InvalidAppliedBy);
	}
	if (!IsValid(Source))
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::InvalidSource);
	}
	if (EventType == EHealthEventType::None || Amount < 0.0f)
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::InvalidEvent);
	}
	bool bCanTakeEventType = false;
	switch (EventType)
	{
    case EHealthEventType::Damage :
    	bCanTakeEventType = bCanEverReceiveDamage;
		break;
    case EHealthEventType::Healing :
    case EHealthEventType::Absorb :
    	bCanTakeEventType = bCanEverReceiveHealing;
		break;
    default :
    	bCanTakeEventType = false;
		break;
	}
	if (!bHasHealth || !bCanTakeEventType)
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::CantTakeEventType);
	}
	if (IsDead())
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::Dead);
	}
	//NPCs resetting or uninitialized cannot take damage.
	if (EventType == EHealthEventType::Damage
		&& IsValid(NPCComponentRef)
		&& !USaiyoraCombatLibrary::IsActiveNPCBehavior(NPCComponentRef->GetCombatBehavior()))
	{
		HealthEvent.Result.FailReasons.AddUnique(EHealthEventFailReason::InactiveNPC);
	}
	//Fail to apply if any validity/generic checks failed.
	//Not realistic to check any of the other conditions with invalid data or blanket restrictions.
	if (HealthEvent.Result.FailReasons.Num() > 0)
	{
		DEBUG_LOG_HEALTH_EVENT_FAIL(this, HealthEvent);
		return HealthEvent;
	}

	UDamageHandler* GeneratorComponent = AppliedBy->Implements<USaiyoraCombatInterface>() ?
		ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy) : nullptr;
	
    if (!bIgnoreModifiers)
    {
    	//Apply outgoing modifiers (and the source modifier, if valid), so long as this isn't an event that already snapshotted outgoing modifiers.
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            HealthEvent.Info.Value = GeneratorComponent->ApplyHealthEventModifiers(ECombatEventDirection::Outgoing, HealthEvent.Info, HealthEvent.Result.Snapshot.AppliedModifiers, SourceModifier);
        	HealthEvent.Result.Snapshot.Value = HealthEvent.Info.Value;
        }
    	//Apply incoming modifiers.
        HealthEvent.Info.Value = ApplyHealthEventModifiers(ECombatEventDirection::Incoming, HealthEvent.Info, HealthEvent.Result.IncomingModifiers);
    }
	if (!bIgnoreRestrictions)
	{
		FHealthEventRestriction IncomingRestriction;
		const bool bIncomingRestricted = CheckEventRestricted(ECombatEventDirection::Incoming, HealthEvent.Info, HealthEvent.Result.FailReasons, IncomingRestriction);
		FHealthEventRestriction OutgoingRestriction;
		const bool bOutgoingRestricted = IsValid(GeneratorComponent) && GeneratorComponent->CheckEventRestricted(ECombatEventDirection::Outgoing, HealthEvent.Info, HealthEvent.Result.FailReasons, OutgoingRestriction);
		if (bIncomingRestricted || bOutgoingRestricted)
		{
			//Use proxy structs to save off the restrictions that prevented this event, due to circular dependency problems.
			HealthEvent.Result.IncomingRestriction = IncomingRestriction;
			HealthEvent.Result.OutgoingRestriction = OutgoingRestriction;
			DEBUG_LOG_HEALTH_EVENT_FAIL(this, HealthEvent);
			return HealthEvent;
		}
	}

	HealthEvent.Result.bSuccess = true;
	const float PreviousAbsorb = CurrentAbsorb;
	const float PreviousHealth = CurrentHealth;
	FDeathRestriction DeathRestriction;
	bool bRestrictedKillingBlow = false;
	bool bAddedPendingKillingBlow = false;
	if (EventType == EHealthEventType::Damage)
	{
		HealthEvent.Result.PreviousValue = CurrentHealth;
		float RemainingDamage = HealthEvent.Info.Value;
		if (!bBypassAbsorbs)
		{
			if (CurrentAbsorb >= RemainingDamage)
			{
				RemainingDamage = 0.0f;
				CurrentAbsorb = FMath::Clamp(CurrentAbsorb - HealthEvent.Info.Value, 0.0f, MaxHealth);
			}
			else
			{
				RemainingDamage -= CurrentAbsorb;
				CurrentAbsorb = 0.0f;
			}
		}
		CurrentHealth = FMath::Clamp(CurrentHealth - RemainingDamage, 0.0f, MaxHealth);
		HealthEvent.Result.NewValue = CurrentHealth;
		HealthEvent.Result.AppliedValue = HealthEvent.Result.PreviousValue - CurrentHealth;
		if (CurrentHealth == 0.0f)
		{
			HealthEvent.Result.bKillingBlow = true;
			if (!bIgnoreDeathRestrictions && DeathRestrictions.IsRestricted(DeathRestriction, HealthEvent))
			{
				HealthEvent.Result.bKillingBlow = false;
				bRestrictedKillingBlow = true;
				//If we don't already have a pending killing blow, set this to be our pending killing blow.
				//Until we are healed above 1 health, any death restriction removal will trigger re-evaluation of the pending killing blow to see if it is now allowed to kill us.
				if (!bHasPendingKillingBlow)
				{
					bHasPendingKillingBlow = true;
					PendingKillingBlow = HealthEvent;
					bAddedPendingKillingBlow = true;
				}
			}
		}
	}
	else if (EventType == EHealthEventType::Healing)
	{
		HealthEvent.Result.PreviousValue = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth + HealthEvent.Info.Value, 0.0f, MaxHealth);
		HealthEvent.Result.NewValue = CurrentHealth;
		HealthEvent.Result.AppliedValue = CurrentHealth - HealthEvent.Result.PreviousValue;
		if (bHasPendingKillingBlow && CurrentHealth > 0.0f)
		{
			bHasPendingKillingBlow = false;
			PendingKillingBlow = FHealthEvent();
		}
	}
	else if (EventType == EHealthEventType::Absorb)
	{
		HealthEvent.Result.PreviousValue = CurrentAbsorb;
		CurrentAbsorb = FMath::Clamp(CurrentAbsorb + HealthEvent.Info.Value, 0.0f, MaxHealth);
		HealthEvent.Result.NewValue = CurrentAbsorb;
		HealthEvent.Result.AppliedValue = CurrentAbsorb - HealthEvent.Result.PreviousValue;
	}

	DEBUG_LOG_HEALTH_EVENT(this, HealthEvent);
	//If this should have been a killing blow, but was restricted, log that restriction and whether this is the new pending killing blow.
	if (bRestrictedKillingBlow)
	{
		DEBUG_LOG_DEATH_RESTRICTED(this, HealthEvent.Info.EventID, false, bAddedPendingKillingBlow, DeathRestriction);
	}

	if (CurrentAbsorb != PreviousAbsorb)
	{
		OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
	}
	if (CurrentHealth != PreviousHealth)
	{
		OnHealthChanged.Broadcast(GetOwner(), PreviousHealth, CurrentHealth);
	}
	OnIncomingHealthEvent.Broadcast(HealthEvent);
	
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfIncomingHealthEvent(HealthEvent);
	}
	if (IsValid(GeneratorComponent))
	{
		GeneratorComponent->NotifyOfOutgoingHealthEvent(HealthEvent);
	}
	if (HealthEvent.Result.bKillingBlow)
	{
		DEBUG_LOG_DEATH(this, HealthEvent.Info.EventID, false);
		Die();
	}
	
	return HealthEvent;
}

FHealthEvent UDamageHandler::ApplyHealthEvent(const EHealthEventType EventType, const float Amount, AActor* AppliedBy,
	UObject* Source, const EEventHitStyle HitStyle, const EElementalSchool School, const bool bBypassAbsorbs,
	const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions, const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams)
{
    return ApplyHealthEvent_Internal(EventType, Amount, AppliedBy, Source, HitStyle, School, bBypassAbsorbs,
    	bIgnoreModifiers, bIgnoreRestrictions, bIgnoreDeathRestrictions, false, FHealthEventSnapshot(), SourceModifier, ThreatParams);
}

FHealthEvent UDamageHandler::ApplySnapshottedHealthEvent(const EHealthEventType EventType, const FHealthEventSnapshot& Snapshot, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
	const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions, const FThreatFromDamage& ThreatParams)
{
	return ApplyHealthEvent_Internal(EventType, Snapshot.Value, AppliedBy, Source, HitStyle, School, bBypassAbsorbs,
		bIgnoreModifiers, bIgnoreRestrictions, bIgnoreDeathRestrictions, true, Snapshot, FHealthEventModCondition(), ThreatParams);
}

void UDamageHandler::NotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingHealthEvent.Broadcast(HealthEvent);
		if (HealthEvent.Result.bKillingBlow)
		{
			OnKillingBlow.Broadcast(HealthEvent);
		}
		if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
		{
			ClientNotifyOfOutgoingHealthEvent(HealthEvent);
		}
	}
}

void UDamageHandler::ClientNotifyOfOutgoingHealthEvent_Implementation(const FHealthEvent& HealthEvent)
{
	OnOutgoingHealthEvent.Broadcast(HealthEvent);
	if (HealthEvent.Result.bKillingBlow)
	{
		OnKillingBlow.Broadcast(HealthEvent);
	}
}

void UDamageHandler::ClientNotifyOfIncomingHealthEvent_Implementation(const FHealthEvent& HealthEvent)
{
	OnIncomingHealthEvent.Broadcast(HealthEvent);
}

#pragma endregion
#pragma region Restrictions

bool UDamageHandler::AddDeathRestriction(const FDeathRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}
	if (!Restriction.IsBound() || !Restriction.GetUObject()->IsA(UBuff::StaticClass()))
	{
		DEBUG_LOG_DEATH_RESTRICTION_ADD_FAILED(this, Restriction, TEXT("Restriction not bound to UBuff function."));
		return false;
	}
	const bool bAdded = DeathRestrictions.Add(Restriction);
	if (bAdded)
	{
		DEBUG_LOG_DEATH_RESTRICTION_ADDED(this, Restriction);
	}
	else
	{
		DEBUG_LOG_DEATH_RESTRICTION_ADD_FAILED(this, Restriction, TEXT("Duplicate restriction in list."));
	}
	return bAdded;
}

void UDamageHandler::RemoveDeathRestriction(const FDeathRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const int NumRemoved = DeathRestrictions.Remove(Restriction);
	if (NumRemoved > 0)
	{
		DEBUG_LOG_DEATH_RESTRICTION_REMOVED(this, Restriction);
		//If a death restriction is removed while we have a pending killing blow, re-evaluate to see if we can now die to it.
		if (bHasPendingKillingBlow)
		{
			FDeathRestriction KillingBlowRestriction;
			if (DeathRestrictions.IsRestricted(KillingBlowRestriction, PendingKillingBlow))
			{
				DEBUG_LOG_DEATH_RESTRICTED(this, PendingKillingBlow.Info.EventID, true, false, KillingBlowRestriction);
			}
			else
			{
				DEBUG_LOG_DEATH(this, PendingKillingBlow.Info.EventID, true);
				Die();
			}
		}
	}
	else
	{
		DEBUG_LOG_DEATH_RESTRICTION_REMOVE_FAILED(this, Restriction, TEXT("Restriction not found in list."));
	}
}

bool UDamageHandler::AddHealthEventRestriction(const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}
	if (!Restriction.IsBound() || !Restriction.GetUObject()->IsA(UBuff::StaticClass()))
	{
		DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADD_FAILED(this, Direction, Restriction, TEXT("Restriction not bound to UBuff function."));
		return false;
	}
	const bool bAdded = Direction == ECombatEventDirection::Incoming
		? IncomingHealthEventRestrictions.Add(Restriction) : OutgoingHealthEventRestrictions.Add(Restriction);
	if (bAdded)
	{
		DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADDED(this, Direction, Restriction);
	}
	else
	{
		DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADD_FAILED(this, Direction, Restriction, TEXT("Duplicate restriction in list."));
	}
	return bAdded;
}

void UDamageHandler::RemoveHealthEventRestriction(const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const int NumRemoved = Direction == ECombatEventDirection::Incoming ? IncomingHealthEventRestrictions.Remove(Restriction) : OutgoingHealthEventRestrictions.Remove(Restriction);
	if (NumRemoved > 0)
	{
		DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVED(this, Direction, Restriction);
	}
	else
	{
		DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVE_FAILED(this, Direction, Restriction, TEXT("Restriction not found in list."));
	}
}

bool UDamageHandler::CheckEventRestricted(const ECombatEventDirection Direction, const FHealthEventInfo& Event, TArray<EHealthEventFailReason>& OutFailReasons, FHealthEventRestriction& OutRestriction)
{
	bool bRestricted = false;
	if (Direction == ECombatEventDirection::Incoming
		? IncomingHealthEventRestrictions.IsRestricted(OutRestriction, Event)
		: OutgoingHealthEventRestrictions.IsRestricted(OutRestriction, Event))
	{
		OutFailReasons.AddUnique(Direction == ECombatEventDirection::Incoming
			? EHealthEventFailReason::IncomingRestriction : EHealthEventFailReason::OutgoingRestriction);
		bRestricted = true;
	}
	return bRestricted;
}

#pragma endregion
#pragma region Modifiers

bool UDamageHandler::AddHealthEventModifier(const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}
	if (!Modifier.IsBound() || !Modifier.GetUObject()->IsA(UBuff::StaticClass()))
	{
		DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADD_FAILED(this, Direction, Modifier, TEXT("Modifier not bound to UBuff function."));
		return false;
	}
	const bool bAdded = Direction == ECombatEventDirection::Incoming
		? IncomingHealthEventModifiers.Add(Modifier) : OutgoingHealthEventModifiers.Add(Modifier);
	if (bAdded)
	{
		DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADDED(this, Direction, Modifier);
	}
	else
	{
		DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADD_FAILED(this, Direction, Modifier, TEXT("Duplicate modifier in list."));
	}
	return bAdded;
}

void UDamageHandler::RemoveHealthEventModifier(const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const int NumRemoved = Direction == ECombatEventDirection::Incoming ? IncomingHealthEventModifiers.Remove(Modifier) : OutgoingHealthEventModifiers.Remove(Modifier);
	if (NumRemoved > 0)
	{
		DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVED(this, Direction, Modifier);
	}
	else
	{
		DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVE_FAILED(this, Direction, Modifier, TEXT("Modifier not found in list."));
	}
}

float UDamageHandler::ApplyHealthEventModifiers(const ECombatEventDirection Direction, const FHealthEventInfo& EventInfo, TArray<FModifierProxy>& OutModConditions,
	const FHealthEventModCondition& SourceMod) const
{
	//Collect the relevant modifiers given the direction of the event and the info about it.
	TArray<FCombatModifier> Mods;
	Direction == ECombatEventDirection::Incoming ? IncomingHealthEventModifiers.GetModifiersWithSources(Mods, OutModConditions, EventInfo)
		: OutgoingHealthEventModifiers.GetModifiersWithSources(Mods, OutModConditions, EventInfo);
	//For outgoing events, we might have a source modifier that should be evaluated as well.
	if (Direction == ECombatEventDirection::Outgoing && SourceMod.IsBound())
	{
		const FCombatModifier& SourceModifier = Mods.Add_GetRef(SourceMod.Execute(EventInfo));
		if (SourceModifier.Type != EModifierType::Invalid)
		{
			//Add the source mod to the list of mod conditions applied, if it generated a valid modifier.
			OutModConditions.Emplace(SourceModifier, SourceMod);
		}
	}
	//Apply the appropriate damage/healing/absorb taken/done stat if we have it.
	if (IsValid(StatHandlerRef))
	{
		const FGameplayTag StatTag = Direction == ECombatEventDirection::Incoming
			//Incoming
			? EventInfo.EventType == EHealthEventType::Damage ? FSaiyoraCombatTags::Get().Stat_DamageTaken
			: EventInfo.EventType == EHealthEventType::Healing ? FSaiyoraCombatTags::Get().Stat_HealingTaken
			: /* EventInfo.EventType == EHealthEventType::Absorb ? */ FSaiyoraCombatTags::Get().Stat_AbsorbTaken
			//Outgoing
			: EventInfo.EventType == EHealthEventType::Damage ? FSaiyoraCombatTags::Get().Stat_DamageDone
			: EventInfo.EventType == EHealthEventType::Healing ? FSaiyoraCombatTags::Get().Stat_HealingDone
			: /* EventInfo.EventType == EHealthEventType::Absorb ? */ FSaiyoraCombatTags::Get().Stat_AbsorbDone;
		if (StatHandlerRef->IsStatValid(StatTag))
		{
			const FCombatModifier& StatModifier = Mods.Add_GetRef(FCombatModifier(StatHandlerRef->GetStatValue(StatTag), EModifierType::Multiplicative));
			if (StatModifier.Value != 1.0f)
			{
				//Include this in the list of applied mod conditions, even though it's not actually a function.
				OutModConditions.Emplace(StatModifier, FCombatDelegateProxy(StatHandlerRef, StatTag.GetTagName()));
			}
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, EventInfo.Value);
}

#pragma endregion
#pragma region DEBUG
#if !UE_BUILD_SHIPPING

void UDamageHandler::DEBUG_DumpState(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const
{
	const FString DirectionString = UEnum::GetDisplayValueAsText(Direction).ToString().ToUpper();
	OutStrings.Add(FString::Printf(L"DAMAGE HANDLER %s STATE DUMP: LifeStatus: %s, CurrentHealth: %.2f, MaxHealth: %.2f, CurrentAbsorb: %.2f, PendingKillingBlow: %s",
		*DirectionString, *UEnum::GetDisplayValueAsText(LifeStatus).ToString(), CurrentHealth, MaxHealth, CurrentAbsorb,
		PendingKillingBlow.Info.EventID == -1 ? TEXT("No") : *FString::FromInt(PendingKillingBlow.Info.EventID)));
}

void UDamageHandler::DEBUG_DumpRestrictions(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const
{
	const FString DirectionString = UEnum::GetDisplayValueAsText(Direction).ToString().ToUpper();
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER %s RESTRICTION DUMP START---", *DirectionString));
	OutStrings.Add(FString::Printf(L"Handler for actor %s has %i restrictions applied.", *GetOwner()->GetName(),
		Direction == ECombatEventDirection::Incoming ? IncomingHealthEventRestrictions.Num() : OutgoingHealthEventRestrictions.Num()));
	const TArray<FHealthEventRestriction>& Restrictions = Direction == ECombatEventDirection::Incoming
		? IncomingHealthEventRestrictions.GetRestrictionsList() : OutgoingHealthEventRestrictions.GetRestrictionsList();
	for (const FHealthEventRestriction& Restriction : Restrictions)
	{
		OutStrings.Add(USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	}
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER %s RESTRICTION DUMP END---", *DirectionString));
}

void UDamageHandler::DEBUG_DumpModifiers(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const
{
	const FString DirectionString = UEnum::GetDisplayValueAsText(Direction).ToString().ToUpper();
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER %s MODIFIER DUMP START---", *DirectionString));
	OutStrings.Add(FString::Printf(L"Handler for actor %s has %i modifiers applied.", *GetOwner()->GetName(),
		Direction == ECombatEventDirection::Incoming ? IncomingHealthEventModifiers.Num() : OutgoingHealthEventModifiers.Num()));
	const TArray<FHealthEventModCondition>& Modifiers = Direction == ECombatEventDirection::Incoming
		? IncomingHealthEventModifiers.GetModifierList() : OutgoingHealthEventModifiers.GetModifierList();
	for (const FHealthEventModCondition& Modifier : Modifiers)
	{
		OutStrings.Add(USaiyoraCombatLibrary::DelegateToLogString(Modifier));
	}
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER %s MODIFIER DUMP END---", *DirectionString));
}

void UDamageHandler::DEBUG_DumpDeathRestrictions(TArray<FString>& OutStrings) const
{
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER DEATH RESTRICTION DUMP START---"));
	OutStrings.Add(FString::Printf(L"Handler for actor %s has %i restrictions applied.", *GetOwner()->GetName(), DeathRestrictions.Num()));
	for (const FDeathRestriction& Restriction : DeathRestrictions.GetRestrictionsList())
	{
		OutStrings.Add(USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	}
	OutStrings.Add(FString::Printf(L"---DAMAGE HANDLER DEATH RESTRICTION DUMP END---"));
}

#endif
#pragma endregion 