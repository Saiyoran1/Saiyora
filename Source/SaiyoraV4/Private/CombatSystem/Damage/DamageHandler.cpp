#include "DamageHandler.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "StatHandler.h"
#include "CombatStatusComponent.h"
#include "DamageBuffFunctions.h"
#include "SaiyoraCombatInterface.h"
#include "DungeonGameState.h"
#include "IAsyncTask.h"
#include "NPCAbilityComponent.h"
#include "UnrealNetwork.h"

#pragma region Initialization

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
	
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Damage Handler."));
	if (GetOwnerRole() == ROLE_Authority)
	{
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
		DisableHealthEvents.BindDynamic(this, &UDamageHandler::DisableAllHealthEvents);
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
			if (NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::None || NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::Resetting)
			{
				AddOutgoingHealthEventRestriction(DisableHealthEvents);
				AddIncomingHealthEventRestriction(DisableHealthEvents);
			}
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
}

void UDamageHandler::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	const bool bWasDisabled = PreviousBehavior == ENPCCombatBehavior::None || PreviousBehavior == ENPCCombatBehavior::Resetting;
	const bool bShouldBeDisabled = NewBehavior == ENPCCombatBehavior::None || NewBehavior == ENPCCombatBehavior::Resetting;
	if (bWasDisabled && !bShouldBeDisabled)
	{
		RemoveOutgoingHealthEventRestriction(DisableHealthEvents);
		RemoveIncomingHealthEventRestriction(DisableHealthEvents);
	}
	else if (!bWasDisabled && bShouldBeDisabled)
	{
		AddOutgoingHealthEventRestriction(DisableHealthEvents);
		AddIncomingHealthEventRestriction(DisableHealthEvents);
		ApplyHealthEvent(EHealthEventType::Healing, MaxHealth, GetOwner(), this, EEventHitStyle::Authority, EElementalSchool::None, true,
			true, true, true, false, FHealthEventModCondition(), FThreatFromDamage());
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
	                 true, bIgnoreDeathRestrictions, false, FHealthEventModCondition(), FThreatFromDamage());
}

void UDamageHandler::Die()
{
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
	const ELifeStatus PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Dead;
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
	if (GameStateRef)
	{
		switch (KillCountType)
		{
		case EKillCountType::None :
			return;
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
			return;
		}
	}
}

void UDamageHandler::RespawnActor(const bool bForceRespawnLocation, const FVector& OverrideRespawnLocation,
	const bool bForceHealthPercentage, const float OverrideHealthPercentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanRespawn || LifeStatus != ELifeStatus::Dead)
	{
		return;
	}
	if (!bRespawnInPlace || bForceRespawnLocation)
	{
		GetOwner()->SetActorLocation(bForceRespawnLocation ? OverrideRespawnLocation : RespawnLocation);
	}
	CurrentHealth = FMath::Clamp(MaxHealth * (bForceHealthPercentage ? FMath::Clamp(OverrideHealthPercentage, 0.0f, 1.0f) : 1.0f), 1.0f, MaxHealth);
	if (CurrentAbsorb != 0.0f)
	{
		const float PreviousAbsorb = CurrentAbsorb;
		CurrentAbsorb = 0.0f;
		OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
	}
	OnHealthChanged.Broadcast(GetOwner(), 0.0f, CurrentHealth);
	const ELifeStatus PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Alive;
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, ELifeStatus::Alive);
}

void UDamageHandler::UpdateRespawnPoint(const FVector& NewLocation)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanRespawn || bRespawnInPlace)
	{
		return;
	}
	RespawnLocation = NewLocation;
}

void UDamageHandler::Server_RequestRespawn_Implementation()
{
	RespawnActor();
}

int UDamageHandler::AddPendingResurrection(UPendingResurrectionFunction* PendingResFunction)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(PendingResFunction) || !bCanRespawn)
	{
		return -1;
	}
	const FPendingResurrection& Res = PendingResurrections.Emplace_GetRef(PendingResFunction);
	OnPendingResAdded.Broadcast(Res);
	return Res.ResID;
}

void UDamageHandler::RemovePendingResurrection(const int ResID)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanRespawn || ResID == -1)
	{
		return;
	}
	FPendingResurrection FoundRes;
	for (int i = PendingResurrections.Num() - 1; i >= 0; i--)
	{
		if (PendingResurrections[i].ResID == ResID)
		{
			FoundRes = PendingResurrections[i];
			PendingResurrections.RemoveAt(i);
			break;
		}
	}
	//If ID != -1, it means we found a valid res from this buff and removed it.
	if (FoundRes.ResID != -1)
	{
		OnPendingResRemoved.Broadcast(FoundRes);
	}
}

void UDamageHandler::OnRep_PendingResurrections(const TArray<FPendingResurrection>& PreviousResurrections)
{
	TArray<FPendingResurrection> Removed;
	TArray<FPendingResurrection> Added;
	for (const FPendingResurrection& PreviousRes : PreviousResurrections)
	{
		if (!PendingResurrections.Contains(PreviousRes))
		{
			Removed.Add(PreviousRes);
		}
	}
	for (const FPendingResurrection& NewRes : PendingResurrections)
	{
		if (!PreviousResurrections.Contains(NewRes))
		{
			Added.Add(NewRes);
		}
	}
	for (const FPendingResurrection& RemovedRes : Removed)
	{
		OnPendingResRemoved.Broadcast(RemovedRes);
	}
	for (const FPendingResurrection& AddedRes : Added)
	{
		OnPendingResAdded.Broadcast(AddedRes);
	}
}

void UDamageHandler::AcceptResurrection(const int ResID)
{
	if (ResID == -1 || !bCanRespawn || LifeStatus != ELifeStatus::Dead)
	{
		return;
	}
	//If this is called by the local player, we'll route to the server to try and accept the resurrection.
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_MakeResDecision(ResID, true);
		return;
	}
	//Otherwise, we need to be the server to be resurrected.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	for (const FPendingResurrection& PendingRes : PendingResurrections)
	{
		if (PendingRes.ResID == ResID)
		{
			//Call the callback function on the source of the res, if one exists.
			//This plays some cosmetic effects when accepting the res.
			if (IsValid(PendingRes.BuffSource))
			{
				if (UFunction* Callback = PendingRes.BuffSource->FindFunction(PendingRes.CallbackName))
				{
					//To pass parameters to a UFunction dynamically, you need to pass a struct pointer containing the params.
					struct
					{
						FVector ResLocation;
					} Params;
					Params.ResLocation = PendingRes.ResLocation;
					//Call the callback function.
					PendingRes.BuffSource->ProcessEvent(Callback, &Params);
				}
			}
			RespawnActor(true, PendingRes.ResLocation, PendingRes.bOverrideHealth, PendingRes.OverrideHealthPercent);
		}
	}
}

void UDamageHandler::DeclineResurrection(const int ResID)
{
	if (ResID == -1 || !bCanRespawn || LifeStatus != ELifeStatus::Dead)
	{
		return;
	}
	//If this is called by the local player, we'll route to the server to try and accept the resurrection.
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_MakeResDecision(ResID, false);
		return;
	}
	//Otherwise, we need to be the server to be resurrected.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	for (int i = PendingResurrections.Num() - 1; i >= 0; i--)
	{
		const FPendingResurrection& PendingRes = PendingResurrections[i];
		if (PendingRes.ResID == ResID)
		{
			if (IsValid(PendingRes.BuffSource))
			{
				PendingRes.BuffSource->GetHandler()->RemoveBuff(PendingRes.BuffSource, EBuffExpireReason::Dispel);
			}
			else
			{
				RemovePendingResurrection(PendingRes.ResID);
			}
			break;
		}
	}
}

void UDamageHandler::Server_MakeResDecision_Implementation(const int ResID, const bool bAccept)
{
	if (bAccept)
	{
		AcceptResurrection(ResID);
	}
	else
	{
		DeclineResurrection(ResID);
	}
}

#pragma endregion 
#pragma region Damage

FHealthEvent UDamageHandler::ApplyHealthEvent(const EHealthEventType EventType, const float Amount, AActor* AppliedBy,
                                              UObject* Source, const EEventHitStyle HitStyle, const EElementalSchool School, const bool bBypassAbsorbs,
                                              const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions, const bool bFromSnapshot, const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams)
{
    FHealthEvent HealthEvent;
    if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !IsValid(Source))
    {
        return HealthEvent;
    }
    if (!bHasHealth || IsDead() || EventType == EHealthEventType::None || Amount < 0.0f)
    {
        return HealthEvent;
    }
	switch (EventType)
	{
    case EHealthEventType::Damage :
    	if (!bCanEverReceiveDamage) return HealthEvent;
		break;
    case EHealthEventType::Healing :
    	if (!bCanEverReceiveHealing) return HealthEvent;
		break;
    case EHealthEventType::Absorb :
    	if (!bCanEverReceiveHealing) return HealthEvent;
		break;
    default :
    	return HealthEvent;
	}
	HealthEvent.Info.EventType = EventType;
    HealthEvent.Info.Value = Amount;
    HealthEvent.Info.SnapshotValue = Amount;
    HealthEvent.Info.AppliedBy = AppliedBy;
    HealthEvent.Info.AppliedTo = GetOwner();
    HealthEvent.Info.Source = Source;
    HealthEvent.Info.HitStyle = HitStyle;
    HealthEvent.Info.School = School;
	if (HealthEvent.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UCombatStatusComponent* AppliedByCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(HealthEvent.Info.AppliedBy);
		HealthEvent.Info.AppliedByPlane = IsValid(AppliedByCombatStatusComp) ? AppliedByCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
	}
	else
	{
		HealthEvent.Info.AppliedByPlane = ESaiyoraPlane::Both;
	}
    HealthEvent.Info.AppliedToPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
    HealthEvent.Info.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(HealthEvent.Info.AppliedByPlane, HealthEvent.Info.AppliedToPlane);
	if (ThreatParams.GeneratesThreat)
	{
		HealthEvent.ThreatInfo = ThreatParams;
		if (!ThreatParams.SeparateBaseThreat)
		{
			HealthEvent.ThreatInfo.BaseThreat = HealthEvent.Info.Value;
		}
	}

	UDamageHandler* GeneratorComponent = AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) ?
		ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy) : nullptr;
	
    if (!bIgnoreModifiers)
    {
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            HealthEvent.Info.Value = GeneratorComponent->GetModifiedOutgoingHealthEventValue(HealthEvent.Info, SourceModifier);
            HealthEvent.Info.SnapshotValue = HealthEvent.Info.Value;
        }
        HealthEvent.Info.Value = GetModifiedIncomingHealthEventValue(HealthEvent.Info);
    }
    if (!bIgnoreRestrictions && (IncomingHealthEventRestrictions.IsRestricted(HealthEvent.Info) || (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingHealthEventRestricted(HealthEvent.Info))))
    {
        return HealthEvent;
    }

	HealthEvent.Result.Success = true;
	if (EventType == EHealthEventType::Damage)
	{
		HealthEvent.Result.PreviousValue = CurrentHealth;
		float RemainingDamage = HealthEvent.Info.Value;
		if (!bBypassAbsorbs)
		{
			if (CurrentAbsorb >= RemainingDamage)
			{
				RemainingDamage = 0.0f;
				const float PreviousAbsorb = CurrentAbsorb;
				CurrentAbsorb = FMath::Clamp(CurrentAbsorb - HealthEvent.Info.Value, 0.0f, MaxHealth);
				if (PreviousAbsorb != CurrentAbsorb)
				{
					OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
				}
			}
			else
			{
				RemainingDamage -= CurrentAbsorb;
				const float PreviousAbsorb = CurrentAbsorb;
				CurrentAbsorb = 0.0f;
				if (PreviousAbsorb != CurrentAbsorb)
				{
					OnAbsorbChanged.Broadcast(GetOwner(), PreviousAbsorb, CurrentAbsorb);
				}
			}
		}
		CurrentHealth = FMath::Clamp(CurrentHealth - RemainingDamage, 0.0f, MaxHealth);
		if (CurrentHealth != HealthEvent.Result.PreviousValue)
		{
			OnHealthChanged.Broadcast(GetOwner(), HealthEvent.Result.PreviousValue, CurrentHealth);
		}
		HealthEvent.Result.NewValue = CurrentHealth;
		HealthEvent.Result.AppliedValue = HealthEvent.Result.PreviousValue - CurrentHealth;
		if (CurrentHealth == 0.0f)
		{
			if (bIgnoreDeathRestrictions || !DeathRestrictions.IsRestricted(HealthEvent))
			{
				HealthEvent.Result.KillingBlow = true;
			}
			else if (!bHasPendingKillingBlow)
			{
				bHasPendingKillingBlow = true;
				PendingKillingBlow = HealthEvent;
			}
		}
	}
	else if (EventType == EHealthEventType::Healing)
	{
		HealthEvent.Result.PreviousValue = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth + HealthEvent.Info.Value, 0.0f, MaxHealth);
		if (CurrentHealth != HealthEvent.Result.PreviousValue)
		{
			OnHealthChanged.Broadcast(GetOwner(), HealthEvent.Result.PreviousValue, CurrentHealth);
		}
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
		if (CurrentAbsorb != HealthEvent.Result.PreviousValue)
		{
			OnAbsorbChanged.Broadcast(GetOwner(), HealthEvent.Result.PreviousValue, CurrentAbsorb);
		}
		HealthEvent.Result.NewValue = CurrentAbsorb;
		HealthEvent.Result.AppliedValue = CurrentAbsorb - HealthEvent.Result.PreviousValue;
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
	if (HealthEvent.Result.KillingBlow)
	{
		Die();
	}
	
	return HealthEvent;
}

void UDamageHandler::NotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingHealthEvent.Broadcast(HealthEvent);
		if (HealthEvent.Result.KillingBlow)
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
	if (HealthEvent.Result.KillingBlow)
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

void UDamageHandler::RemoveDeathRestriction(const FDeathRestriction& Restriction)
{
	if (DeathRestrictions.Remove(Restriction) > 0 && bHasPendingKillingBlow)
	{
		if (!DeathRestrictions.IsRestricted(PendingKillingBlow))
		{
			Die();
		}
	}
}

#pragma endregion
#pragma region Modifiers

float UDamageHandler::GetModifiedOutgoingHealthEventValue(const FHealthEventInfo& EventInfo, const FHealthEventModCondition& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	OutgoingHealthEventModifiers.GetModifiers(Mods, EventInfo);
	if (SourceMod.IsBound())
	{
		Mods.Add(SourceMod.Execute(EventInfo));
	}
	if (IsValid(StatHandlerRef))
	{
		switch (EventInfo.EventType)
		{
		case EHealthEventType::Damage :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_DamageDone))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_DamageDone), EModifierType::Multiplicative));
			}
			break;
		case EHealthEventType::Healing :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_HealingDone))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_HealingDone), EModifierType::Multiplicative));
			}
			break;
		case EHealthEventType::Absorb :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_AbsorbDone))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_AbsorbDone), EModifierType::Multiplicative));
			}
			break;
		default :
			break;
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, EventInfo.Value);
}

float UDamageHandler::GetModifiedIncomingHealthEventValue(const FHealthEventInfo& EventInfo) const
{
	TArray<FCombatModifier> Mods;
	IncomingHealthEventModifiers.GetModifiers(Mods, EventInfo);
	if (IsValid(StatHandlerRef))
	{
		switch (EventInfo.EventType)
		{
		case EHealthEventType::Damage :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_DamageTaken))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_DamageTaken), EModifierType::Multiplicative));
			}
			break;
		case EHealthEventType::Healing :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_HealingTaken))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_HealingTaken), EModifierType::Multiplicative));
			}
			break;
		case EHealthEventType::Absorb :
			if (StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_AbsorbTaken))
			{
				Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_AbsorbTaken), EModifierType::Multiplicative));
			}
			break;
		default :
			break;
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, EventInfo.Value);
}

#pragma endregion