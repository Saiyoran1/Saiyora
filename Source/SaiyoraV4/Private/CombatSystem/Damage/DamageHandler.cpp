#include "DamageHandler.h"
#include "StatHandler.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"
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
	if (bHasHealth)
	{
		MaxHealth = DefaultMaxHealth;
		CurrentHealth = MaxHealth;
	}
	LifeStatus = ELifeStatus::Invalid;
	if (GetOwnerRole() == ROLE_Authority)
	{
		MaxHealthStatCallback.BindDynamic(this, &UDamageHandler::ReactToMaxHealthStat);
	}
}

void UDamageHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Damage Handler."));
	OwnerAsPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerAsPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner of DamageHandler is not a pawn?"));
	}
	if (!GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner of DamageHandler doesn't implement combat interface?"));
	}
	StatHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Bind Max Health stat, create damage and healing modifiers from stats.
		if (IsValid(StatHandler))
		{
			if (bHasHealth && !bStaticMaxHealth && StatHandler->IsStatValid(MaxHealthStatTag()))
			{
				UpdateMaxHealth(StatHandler->GetStatValue(MaxHealthStatTag()));
				StatHandler->SubscribeToStatChanged(MaxHealthStatTag(), MaxHealthStatCallback);
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

void UDamageHandler::UpdateMaxHealth(float const NewMaxHealth)
{
	if (!bHasHealth)
	{
		return;
	}
	float const PreviousMaxHealth = MaxHealth;
	MaxHealth = bStaticMaxHealth ? FMath::Max(1.0f, DefaultMaxHealth) : FMath::Max(1.0f, NewMaxHealth);
	if (MaxHealth != PreviousMaxHealth)
	{
		float const PreviousHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp((CurrentHealth / PreviousMaxHealth) * MaxHealth, 0.0f, MaxHealth);
		if (CurrentHealth != PreviousHealth)
		{
			OnHealthChanged.Broadcast(GetOwner(), PreviousHealth, CurrentHealth);
		}
		OnMaxHealthChanged.Broadcast(GetOwner(), PreviousMaxHealth, MaxHealth);
	}
}

void UDamageHandler::ReactToMaxHealthStat(FGameplayTag const& StatTag, float const NewValue)
{
	if (StatTag.IsValid() && StatTag.MatchesTagExact(MaxHealthStatTag()))
	{
		UpdateMaxHealth(NewValue);
	}
}

void UDamageHandler::Die()
{
	ELifeStatus const PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Dead;
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
}

void UDamageHandler::OnRep_CurrentHealth(float const PreviousValue)
{
	OnHealthChanged.Broadcast(GetOwner(), PreviousValue, CurrentHealth);
}

void UDamageHandler::OnRep_MaxHealth(float const PreviousValue)
{
	OnMaxHealthChanged.Broadcast(GetOwner(), PreviousValue, MaxHealth);
}

void UDamageHandler::OnRep_LifeStatus(ELifeStatus const PreviousValue)
{
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousValue, LifeStatus);
}

#pragma endregion
#pragma region Damage

FDamagingEvent UDamageHandler::ApplyDamage(float const Amount, AActor* AppliedBy, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
	bool const bFromSnapshot, FDamageModCondition const& SourceModifier, FThreatFromDamage const& ThreatParams)
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
		UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(DamageEvent.Info.AppliedBy);
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
		if (!CheckDeathRestricted(DamageEvent))
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
	if (!OwnerAsPawn->IsLocallyControlled())
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

void UDamageHandler::NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingDamage.Broadcast(DamageEvent);
		if (DamageEvent.Result.KillingBlow)
		{
			OnKillingBlow.Broadcast(DamageEvent);
		}
		if (!OwnerAsPawn->IsLocallyControlled())
		{
			ClientNotifyOfOutgoingDamage(DamageEvent);
		}
	}
}

void UDamageHandler::ClientNotifyOfOutgoingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
	if (DamageEvent.Result.KillingBlow)
	{
		OnKillingBlow.Broadcast(DamageEvent);
	}
}

void UDamageHandler::ClientNotifyOfIncomingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnIncomingDamage.Broadcast(DamageEvent);
}

#pragma endregion
#pragma region Healing

FDamagingEvent UDamageHandler::ApplyHealing(float const Amount, AActor* AppliedBy, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
	bool const bFromSnapshot, FDamageModCondition const& SourceModifier, FThreatFromDamage const& ThreatParams)
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
		UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(HealingEvent.Info.AppliedBy);
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
	if (!OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfIncomingHealing(HealingEvent);
	}
	if (IsValid(GeneratorComponent))
	{
		GeneratorComponent->NotifyOfOutgoingHealing(HealingEvent);
	}
	return HealingEvent;
}

void UDamageHandler::NotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		OnOutgoingHealing.Broadcast(HealingEvent);
		if (!OwnerAsPawn->IsLocallyControlled())
		{
			ClientNotifyOfOutgoingHealing(HealingEvent);
		}
	}
}

void UDamageHandler::ClientNotifyOfOutgoingHealing_Implementation(FDamagingEvent const& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
}

void UDamageHandler::ClientNotifyOfIncomingHealing_Implementation(FDamagingEvent const& HealingEvent)
{
	OnIncomingHealing.Broadcast(HealingEvent);
}

#pragma endregion 
#pragma region Subscriptions

void UDamageHandler::SubscribeToHealthChanged(FHealthChangeCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnHealthChanged.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromHealthChanged(FHealthChangeCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnHealthChanged.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToMaxHealthChanged(FHealthChangeCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnMaxHealthChanged.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromMaxHealthChanged(FHealthChangeCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnMaxHealthChanged.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToLifeStatusChanged(FLifeStatusCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnLifeStatusChanged.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromLifeStatusChanged(FLifeStatusCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnLifeStatusChanged.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToOutgoingDamage(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingDamage.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromOutgoingDamage(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingDamage.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToKillingBlow(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnKillingBlow.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromKillingBlow(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnKillingBlow.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToOutgoingHealing(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingHealing.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromOutgoingHealing(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingHealing.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToIncomingDamage(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingDamage.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromIncomingDamage(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingDamage.Remove(Callback);
	}
}

void UDamageHandler::SubscribeToIncomingHealing(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingHealing.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromIncomingHealingSuccess(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingHealing.Remove(Callback);
	}
}

#pragma endregion
#pragma region Restrictions

void UDamageHandler::AddDeathRestriction(UBuff* Source, FDeathRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		DeathRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveDeathRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		int32 const Removed = DeathRestrictions.Remove(Source);
		if (Removed > 0 && bHasPendingKillingBlow)
		{
			if (!CheckDeathRestricted(PendingKillingBlow))
			{
				Die();
			}
		}
	}
}

bool UDamageHandler::CheckDeathRestricted(FDamagingEvent const& DamageEvent)
{
	for (TTuple<UBuff*, FDeathRestriction> const& Restriction : DeathRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageEvent))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingDamageRestriction(UBuff* Source, FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingDamageRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveOutgoingDamageRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingDamageRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (TTuple<UBuff*, FDamageRestriction> const& Restriction : OutgoingDamageRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingHealingRestriction(UBuff* Source, FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		OutgoingHealingRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveOutgoingHealingRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingHealingRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo)
{
	for (TTuple<UBuff*, FDamageRestriction> const& Restriction : OutgoingHealingRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingDamageRestriction(UBuff* Source, FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingDamageRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveIncomingDamageRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingDamageRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (TTuple<UBuff*, FDamageRestriction> const& Restriction : IncomingDamageRestrictions)
	{
		if (Restriction.Value.IsBound() && Restriction.Value.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingHealingRestriction(UBuff* Source, FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		IncomingHealingRestrictions.Add(Source, Restriction);
	}
}

void UDamageHandler::RemoveIncomingHealingRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingHealingRestrictions.Remove(Source);
	}
}

bool UDamageHandler::CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo)
{
	for (TTuple<UBuff*, FDamageRestriction> const& Restriction : IncomingHealingRestrictions)
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

void UDamageHandler::AddOutgoingDamageModifier(UBuff* Source, FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		OutgoingDamageModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveOutgoingDamageModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingDamageModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedOutgoingDamage(FDamageInfo const& DamageInfo, FDamageModCondition const& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FDamageModCondition> const& Modifier : OutgoingDamageModifiers)
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
	if (IsValid(StatHandler) && StatHandler->IsStatValid(DamageDoneStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(DamageDoneStatTag()), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddOutgoingHealingModifier(UBuff* Source, FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		OutgoingHealingModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveOutgoingHealingModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		OutgoingHealingModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedOutgoingHealing(FDamageInfo const& HealingInfo, FDamageModCondition const& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FDamageModCondition> const& Modifier : OutgoingHealingModifiers)
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
	if (IsValid(StatHandler) && StatHandler->IsStatValid(HealingDoneStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(HealingDoneStatTag()), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

void UDamageHandler::AddIncomingDamageModifier(UBuff* Source, FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		IncomingDamageModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveIncomingDamageModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingDamageModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedIncomingDamage(FDamageInfo const& DamageInfo) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FDamageModCondition> const& Modifier : IncomingDamageModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(DamageInfo));
		}
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(DamageTakenStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(DamageTakenStatTag()), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddIncomingHealingModifier(UBuff* Source, FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		IncomingHealingModifiers.Add(Source, Modifier);
	}
}

void UDamageHandler::RemoveIncomingHealingModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		IncomingHealingModifiers.Remove(Source);
	}
}

float UDamageHandler::GetModifiedIncomingHealing(FDamageInfo const& HealingInfo) const
{
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FDamageModCondition> const& Modifier : IncomingHealingModifiers)
	{
		if (Modifier.Value.IsBound())
		{
			Mods.Add(Modifier.Value.Execute(HealingInfo));
		}
	}
	if (IsValid(StatHandler) && StatHandler->IsStatValid(HealingTakenStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandler->GetStatValue(HealingTakenStatTag()), EModifierType::Multiplicative));
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

#pragma endregion