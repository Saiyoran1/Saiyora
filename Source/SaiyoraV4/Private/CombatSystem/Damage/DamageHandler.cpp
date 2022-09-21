#include "DamageHandler.h"
#include "StatHandler.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "DungeonGameState.h"
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
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Damage Handler."));
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	if (bHasHealth)
	{
		MaxHealth = DefaultMaxHealth;
		CurrentHealth = MaxHealth;
	}
	LifeStatus = ELifeStatus::Invalid;
	MaxHealthStatCallback.BindDynamic(this, &UDamageHandler::ReactToMaxHealthStat);
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
		//Bind Max Health stat, create damage and healing modifiers from stats.
		if (IsValid(StatHandlerRef))
		{
			if (bHasHealth && !bStaticMaxHealth && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_MaxHealth))
			{
				UpdateMaxHealth(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_MaxHealth));
				StatHandlerRef->SubscribeToStatChanged(FSaiyoraCombatTags::Get().Stat_MaxHealth, MaxHealthStatCallback);
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
		CurrentHealth = FMath::Clamp((CurrentHealth / PreviousMaxHealth) * MaxHealth, 0.0f, MaxHealth);
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

void UDamageHandler::KillActor(AActor* Attacker, UObject* Source, const bool bIgnoreDeathRestrictions)
{
	ApplyHealthEvent(EHealthEventType::Damage, CurrentHealth, Attacker, Source, EEventHitStyle::Authority, EHealthEventSchool::None, true, true,
	                 true, bIgnoreDeathRestrictions, false, FHealthEventModCondition(), FThreatFromDamage());
}

void UDamageHandler::RespawnActor(const bool bForceRespawnLocation, const FVector& OverrideRespawnLocation,
	const bool bForceHealthPercentage, const float OverrideHealthPercentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanRespawn || LifeStatus == ELifeStatus::Alive)
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

void UDamageHandler::OnRep_LifeStatus(const ELifeStatus PreviousValue)
{
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousValue, LifeStatus);
}

#pragma endregion
#pragma region Damage

FHealthEvent UDamageHandler::ApplyHealthEvent(const EHealthEventType EventType, const float Amount, AActor* AppliedBy,
                                              UObject* Source, const EEventHitStyle HitStyle, const EHealthEventSchool School, const bool bBypassAbsorbs,
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
		HealthEvent.Info.AppliedByPlane = IsValid(AppliedByCombatStatusComp) ? AppliedByCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::None;
	}
	else
	{
		HealthEvent.Info.AppliedByPlane = ESaiyoraPlane::None;
	}
    HealthEvent.Info.AppliedToPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::None;
    HealthEvent.Info.AppliedXPlane = UCombatStatusComponent::CheckForXPlane(HealthEvent.Info.AppliedByPlane, HealthEvent.Info.AppliedToPlane);
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