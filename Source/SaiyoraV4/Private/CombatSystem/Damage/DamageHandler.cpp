#include "DamageHandler.h"
#include "StatHandler.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "PlaneComponent.h"
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
	StatHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
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
		if (IsValid(StatHandler))
		{
			if (bHasHealth && !bStaticMaxHealth && StatHandler->IsStatValid(FSaiyoraCombatTags::Get().Stat_MaxHealth))
			{
				UpdateMaxHealth(StatHandler->GetStatValue(FSaiyoraCombatTags::Get().Stat_MaxHealth));
				StatHandler->SubscribeToStatChanged(FSaiyoraCombatTags::Get().Stat_MaxHealth, MaxHealthStatCallback);
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
	ApplyDamage(CurrentHealth, Attacker, Source, EDamageHitStyle::Authority, EDamageSchool::None, true, true, bIgnoreDeathRestrictions,
	            false, FDamageModCondition(), FThreatFromDamage());
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

void UDamageHandler::OnRep_LifeStatus(const ELifeStatus PreviousValue)
{
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousValue, LifeStatus);
}

#pragma endregion
#pragma region Damage

FDamagingEvent UDamageHandler::ApplyDamage(const float Amount, AActor* AppliedBy, UObject* Source,
                                           const EDamageHitStyle HitStyle, const EDamageSchool School, const bool bIgnoreModifiers, const bool bIgnoreRestrictions,
                                           const bool bIgnoreDeathRestrictions, const bool bFromSnapshot, const FDamageModCondition& SourceModifier, const FThreatFromDamage& ThreatParams)
{
    FDamagingEvent DamageEvent;
    if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !IsValid(Source))
    {
        return DamageEvent;
    }
    if (!bHasHealth || !bCanEverReceiveDamage || IsDead())
    {
        return DamageEvent;
    }
    DamageEvent.Info.Value = Amount;
    DamageEvent.Info.SnapshotValue = Amount;
    DamageEvent.Info.AppliedBy = AppliedBy;
    DamageEvent.Info.AppliedTo = GetOwner();
    DamageEvent.Info.Source = Source;
    DamageEvent.Info.HitStyle = HitStyle;
    DamageEvent.Info.School = School;
	if (DamageEvent.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(DamageEvent.Info.AppliedBy);
		DamageEvent.Info.AppliedByPlane = IsValid(AppliedByPlaneComp) ? AppliedByPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
	}
	else
	{
		DamageEvent.Info.AppliedByPlane = ESaiyoraPlane::None;
	}
    DamageEvent.Info.AppliedToPlane = IsValid(PlaneComponent) ? PlaneComponent->GetCurrentPlane() : ESaiyoraPlane::None;
    DamageEvent.Info.AppliedXPlane = UPlaneComponent::CheckForXPlane(DamageEvent.Info.AppliedByPlane, DamageEvent.Info.AppliedToPlane);
	if (ThreatParams.GeneratesThreat)
	{
		DamageEvent.ThreatInfo = ThreatParams;
		if (!ThreatParams.SeparateBaseThreat)
		{
			DamageEvent.ThreatInfo.BaseThreat = DamageEvent.Info.Value;
		}
	}

	UDamageHandler* GeneratorComponent = AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) ?
		ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy) : nullptr;
	
    if (!bIgnoreModifiers)
    {
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            DamageEvent.Info.Value = GeneratorComponent->GetModifiedOutgoingDamage(DamageEvent.Info, SourceModifier);
            DamageEvent.Info.SnapshotValue = DamageEvent.Info.Value;
        }
        DamageEvent.Info.Value = GetModifiedIncomingDamage(DamageEvent.Info);
    }
    if (!bIgnoreRestrictions && (CheckIncomingDamageRestricted(DamageEvent.Info) || (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingDamageRestricted(DamageEvent.Info))))
    {
        return DamageEvent;
    }
	
	DamageEvent.Result.PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageEvent.Info.Value, 0.0f, MaxHealth);
	if (CurrentHealth != DamageEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(GetOwner(), DamageEvent.Result.PreviousHealth, CurrentHealth);
	}
	DamageEvent.Result.Success = true;
	DamageEvent.Result.NewHealth = CurrentHealth;
	DamageEvent.Result.AmountDealt = DamageEvent.Result.PreviousHealth - CurrentHealth;
	if (CurrentHealth == 0.0f)
	{
		if (bIgnoreDeathRestrictions || !CheckDeathRestricted(DamageEvent))
		{
			DamageEvent.Result.KillingBlow = true;
		}
		else if (!bHasPendingKillingBlow)
		{
			bHasPendingKillingBlow = true;
			PendingKillingBlow = DamageEvent;
		}
	}
	OnIncomingDamage.Broadcast(DamageEvent);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfIncomingDamage(DamageEvent);
	}
	if (IsValid(GeneratorComponent))
	{
		GeneratorComponent->NotifyOfOutgoingDamage(DamageEvent);
	}
	if (DamageEvent.Result.KillingBlow)
	{
		Die();
	}
	
	return DamageEvent;
}

void UDamageHandler::NotifyOfOutgoingDamage(const FDamagingEvent& DamageEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingDamage.Broadcast(DamageEvent);
		if (DamageEvent.Result.KillingBlow)
		{
			OnKillingBlow.Broadcast(DamageEvent);
		}
		if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
		{
			ClientNotifyOfOutgoingDamage(DamageEvent);
		}
	}
}

void UDamageHandler::ClientNotifyOfOutgoingDamage_Implementation(const FDamagingEvent& DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
	if (DamageEvent.Result.KillingBlow)
	{
		OnKillingBlow.Broadcast(DamageEvent);
	}
}

void UDamageHandler::ClientNotifyOfIncomingDamage_Implementation(const FDamagingEvent& DamageEvent)
{
	OnIncomingDamage.Broadcast(DamageEvent);
}

#pragma endregion
#pragma region Healing

FDamagingEvent UDamageHandler::ApplyHealing(const float Amount, AActor* AppliedBy, UObject* Source,
	const EDamageHitStyle HitStyle, const EDamageSchool School, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
	const bool bFromSnapshot, const FDamageModCondition& SourceModifier, const FThreatFromDamage& ThreatParams)
{
	FDamagingEvent HealingEvent;
    if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !IsValid(Source))
    {
        return HealingEvent;
    }
    if (!bHasHealth || !bCanEverReceiveHealing || IsDead())
    {
        return HealingEvent;
    }
    HealingEvent.Info.Value = Amount;
    HealingEvent.Info.SnapshotValue = Amount;
    HealingEvent.Info.AppliedBy = AppliedBy;
    HealingEvent.Info.AppliedTo = GetOwner();
    HealingEvent.Info.Source = Source;
    HealingEvent.Info.HitStyle = HitStyle;
    HealingEvent.Info.School = School;
	if (HealingEvent.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(HealingEvent.Info.AppliedBy);
		HealingEvent.Info.AppliedByPlane = IsValid(AppliedByPlaneComp) ? AppliedByPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
	}
	else
	{
		HealingEvent.Info.AppliedByPlane = ESaiyoraPlane::None;
	}
	HealingEvent.Info.AppliedToPlane = IsValid(PlaneComponent) ? PlaneComponent->GetCurrentPlane() : ESaiyoraPlane::None;
	HealingEvent.Info.AppliedXPlane = UPlaneComponent::CheckForXPlane(HealingEvent.Info.AppliedByPlane, HealingEvent.Info.AppliedToPlane);
	if (ThreatParams.GeneratesThreat)
	{
		HealingEvent.ThreatInfo = ThreatParams;
		if (!ThreatParams.SeparateBaseThreat)
		{
			HealingEvent.ThreatInfo.BaseThreat = HealingEvent.Info.Value;
		}
	}
	
	UDamageHandler* GeneratorComponent = AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) ?
		ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy) : nullptr;
	
    if (!bIgnoreModifiers)
    {
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            HealingEvent.Info.Value = GeneratorComponent->GetModifiedOutgoingHealing(HealingEvent.Info, SourceModifier);
            HealingEvent.Info.SnapshotValue = HealingEvent.Info.Value;
        }
        HealingEvent.Info.Value = GetModifiedIncomingHealing(HealingEvent.Info);
    }
    if (!bIgnoreRestrictions && (CheckIncomingHealingRestricted(HealingEvent.Info) || (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingHealingRestricted(HealingEvent.Info))))
    {
		return HealingEvent;
    }
	HealingEvent.Result.PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealingEvent.Info.Value, 0.0f, MaxHealth);
	if (CurrentHealth != HealingEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(GetOwner(), HealingEvent.Result.PreviousHealth, CurrentHealth);
	}
	if (bHasPendingKillingBlow && CurrentHealth > 0.0f)
	{
		bHasPendingKillingBlow = false;
		PendingKillingBlow = FDamagingEvent();
	}
	HealingEvent.Result.Success = true;
	HealingEvent.Result.NewHealth = CurrentHealth;
	HealingEvent.Result.AmountDealt = CurrentHealth - HealingEvent.Result.PreviousHealth;
	HealingEvent.Result.KillingBlow = false;
	OnIncomingHealing.Broadcast(HealingEvent);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfIncomingHealing(HealingEvent);
	}
	if (IsValid(GeneratorComponent))
	{
		GeneratorComponent->NotifyOfOutgoingHealing(HealingEvent);
	}
	return HealingEvent;
}

void UDamageHandler::NotifyOfOutgoingHealing(const FDamagingEvent& HealingEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingHealing.Broadcast(HealingEvent);
		if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
		{
			ClientNotifyOfOutgoingHealing(HealingEvent);
		}
	}
}

void UDamageHandler::ClientNotifyOfOutgoingHealing_Implementation(const FDamagingEvent& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
}

void UDamageHandler::ClientNotifyOfIncomingHealing_Implementation(const FDamagingEvent& HealingEvent)
{
	OnIncomingHealing.Broadcast(HealingEvent);
}

#pragma endregion 
#pragma region Restrictions

void UDamageHandler::AddDeathRestriction(UBuff* Source, const FDeathRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		DeathRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveDeathRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		if (DeathRestrictions.Remove(Source) > 0 && bHasPendingKillingBlow)
		{
			if (!CheckDeathRestricted(PendingKillingBlow))
			{
				Die();
			}
		}
	}
}

bool UDamageHandler::CheckDeathRestricted(const FDamagingEvent& DamageEvent)
{
	for (const TTuple<UBuff*, FDeathRestriction>& Restriction : DeathRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageEvent))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingDamageRestriction(UBuff* Source, const FDamageRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingDamageRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveOutgoingDamageRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingDamageRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckOutgoingDamageRestricted(const FDamageInfo& DamageInfo)
{
	for (const TTuple<UBuff*, FDamageRestriction>& Restriction : OutgoingDamageRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingHealingRestriction(UBuff* Source, const FDamageRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingHealingRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveOutgoingHealingRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingHealingRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckOutgoingHealingRestricted(const FDamageInfo& HealingInfo)
{
	for (const TTuple<UBuff*, FDamageRestriction>& Restriction : OutgoingHealingRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingDamageRestriction(UBuff* Source, const FDamageRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingDamageRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveIncomingDamageRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingDamageRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckIncomingDamageRestricted(const FDamageInfo& DamageInfo)
{
	for (const TTuple<UBuff*, FDamageRestriction>& Restriction : IncomingDamageRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingHealingRestriction(UBuff* Source, const FDamageRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingHealingRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveIncomingHealingRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingHealingRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckIncomingHealingRestricted(const FDamageInfo& HealingInfo)
{
	for (const TTuple<UBuff*, FDamageRestriction>& Restriction : IncomingHealingRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UDamageHandler::AddOutgoingDamageModifier(UBuff* Source, const FDamageModCondition& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		OutgoingDamageModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveOutgoingDamageModifier(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingDamageModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedOutgoingDamage(const FDamageInfo& DamageInfo, const FDamageModCondition& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (const TTuple<UBuff*, FDamageModCondition>& Modifier : OutgoingDamageModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(DamageInfo));
		}
	}
	if (SourceMod.IsBound())
	{
		Mods.Add(SourceMod.Execute(DamageInfo));
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(FSaiyoraCombatTags::Get().Stat_DamageDone))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(FSaiyoraCombatTags::Get().Stat_DamageDone), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddOutgoingHealingModifier(UBuff* Source, const FDamageModCondition& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		OutgoingHealingModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveOutgoingHealingModifier(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingHealingModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedOutgoingHealing(const FDamageInfo& HealingInfo, const FDamageModCondition& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (const TTuple<UBuff*, FDamageModCondition>& Modifier : OutgoingHealingModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(HealingInfo));
		}
	}
	if (SourceMod.IsBound())
	{
		Mods.Add(SourceMod.Execute(HealingInfo));
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(FSaiyoraCombatTags::Get().Stat_HealingDone))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(FSaiyoraCombatTags::Get().Stat_HealingDone), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

void UDamageHandler::AddIncomingDamageModifier(UBuff* Source, const FDamageModCondition& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		IncomingDamageModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveIncomingDamageModifier(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingDamageModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedIncomingDamage(const FDamageInfo& DamageInfo) const
{
	TArray<FCombatModifier> Mods;
	for (const TTuple<UBuff*, FDamageModCondition>& Modifier : IncomingDamageModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(DamageInfo));
		}
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(FSaiyoraCombatTags::Get().Stat_DamageTaken))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(FSaiyoraCombatTags::Get().Stat_DamageTaken), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddIncomingHealingModifier(UBuff* Source, const FDamageModCondition& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		IncomingHealingModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveIncomingHealingModifier(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingHealingModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedIncomingHealing(const FDamageInfo& HealingInfo) const
{
	TArray<FCombatModifier> Mods;
	for (const TTuple<UBuff*, FDamageModCondition>& Modifier : IncomingHealingModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(HealingInfo));
		}
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(FSaiyoraCombatTags::Get().Stat_HealingTaken))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(FSaiyoraCombatTags::Get().Stat_HealingTaken), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

#pragma endregion