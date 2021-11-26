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
		DamageDoneModFromStat.BindDynamic(this, &UDamageHandler::ModifyDamageDoneFromStat);
		DamageTakenModFromStat.BindDynamic(this, &UDamageHandler::ModifyDamageTakenFromStat);
		HealingDoneModFromStat.BindDynamic(this, &UDamageHandler::ModifyHealingDoneFromStat);
		HealingTakenModFromStat.BindDynamic(this, &UDamageHandler::ModifyHealingTakenFromStat);
		DamageBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictDamageBuffs);
		HealingBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictHealingBuffs);
	}
}

void UDamageHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Damage Handler."), *GetOwner()->GetActorLabel());
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
	BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Bind Max Health stat, create damage and healing modifiers from stats.
		if (IsValid(StatHandler))
		{
			if (bHasHealth && !bStaticMaxHealth && StatHandler->GetStatValid(MaxHealthStatTag()))
			{
				UpdateMaxHealth(StatHandler->GetStatValue(MaxHealthStatTag()));
				StatHandler->SubscribeToStatChanged(MaxHealthStatTag(), MaxHealthStatCallback);
			}
			if (StatHandler->GetStatValid(DamageDoneStatTag()))
			{
				AddOutgoingDamageModifier(DamageDoneModFromStat);
			}
			if (bHasHealth && bCanEverReceiveDamage && StatHandler->GetStatValid(DamageTakenStatTag()))
			{
				AddIncomingDamageModifier(DamageTakenModFromStat);
			}
			if (StatHandler->GetStatValid(HealingDoneStatTag()))
			{
				AddOutgoingHealingModifier(HealingDoneModFromStat);
			}
			if (bHasHealth && bCanEverReceiveHealing && StatHandler->GetStatValid(HealingTakenStatTag()))
			{
				AddIncomingHealingModifier(HealingTakenModFromStat);
			}
		}
		//Add buff restrictions for damage and healing interactions that are not enabled.
		if (IsValid(BuffHandler))
		{
			if (!bHasHealth || !bCanEverReceiveDamage)
			{
				BuffHandler->AddIncomingBuffRestriction(DamageBuffRestriction);
			}
			if (!bHasHealth || !bCanEverReceiveHealing)
			{	
				BuffHandler->AddIncomingBuffRestriction(HealingBuffRestriction);
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

bool UDamageHandler::RestrictDamageBuffs(FBuffApplyEvent const& BuffEvent)
{
	UBuff const* Buff = BuffEvent.BuffClass.GetDefaultObject();
	if (IsValid(Buff))
	{
		FGameplayTagContainer BuffTags;
		Buff->GetBuffTags(BuffTags);
		if (BuffTags.HasTag(BuffFunctionDamageTag()))
		{
			return true;
		}
	}
	return false;
}

bool UDamageHandler::RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent)
{
	UBuff const* Buff = BuffEvent.BuffClass.GetDefaultObject();
	if (IsValid(Buff))
	{
		FGameplayTagContainer BuffFunctionTags;
		Buff->GetBuffTags(BuffFunctionTags);
		if (BuffFunctionTags.HasTag(BuffFunctionHealingTag()))
		{
			return true;
		}
	}
	return false;
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
	if (StatTag.IsValid() && StatTag.MatchesTag(MaxHealthStatTag()))
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

void UDamageHandler::UnsubscribeFromIncomingDamageSuccess(FDamageEventCallback const& Callback)
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

void UDamageHandler::AddDeathRestriction(FDeathRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		DeathRestrictions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveDeathRestriction(FDeathRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		int32 const Removed = DeathRestrictions.Remove(Restriction);
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
	for (FDeathRestriction const& Restriction : DeathRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(DamageEvent))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingDamageRestrictions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingDamageRestrictions.Remove(Restriction);
	}
}

bool UDamageHandler::CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageRestriction const& Restriction : OutgoingDamageRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddOutgoingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingHealingRestrictions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingHealingRestrictions.RemoveSingleSwap(Restriction);
	}
}

bool UDamageHandler::CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo)
{
	for (FDamageRestriction const& Restriction : OutgoingHealingRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingDamageRestrictions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingDamageRestrictions.Remove(Restriction);
	}
}

bool UDamageHandler::CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageRestriction const& Restriction : IncomingDamageRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::AddIncomingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingHealingRestrictions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingHealingRestrictions.Remove(Restriction);
	}
}

bool UDamageHandler::CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo)
{
	for (FDamageRestriction const& Restriction : IncomingHealingRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UDamageHandler::AddOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingDamageModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingDamageModifiers.Remove(Modifier);
	}
}

FCombatModifier UDamageHandler::ModifyDamageDoneFromStat(FDamageInfo const& DamageInfo)
{
	if (IsValid(StatHandler) && StatHandler->GetStatValid(DamageDoneStatTag()))
	{
		return FCombatModifier(StatHandler->GetStatValue(DamageDoneStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UDamageHandler::GetModifiedOutgoingDamage(FDamageInfo const& DamageInfo, FDamageModCondition const& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : OutgoingDamageModifiers)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(DamageInfo));
		}
	}
	if (SourceMod.IsBound())
	{
		Mods.Add(SourceMod.Execute(DamageInfo));
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddOutgoingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingHealingModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveOutgoingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingHealingModifiers.Remove(Modifier);
	}
}

FCombatModifier UDamageHandler::ModifyHealingDoneFromStat(FDamageInfo const& HealingInfo)
{
	if (IsValid(StatHandler) && StatHandler->GetStatValid(HealingDoneStatTag()))
	{
		return FCombatModifier(StatHandler->GetStatValue(HealingDoneStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UDamageHandler::GetModifiedOutgoingHealing(FDamageInfo const& HealingInfo, FDamageModCondition const& SourceMod) const
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : OutgoingHealingModifiers)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(HealingInfo));
		}
	}
	if (SourceMod.IsBound())
	{
		Mods.Add(SourceMod.Execute(HealingInfo));
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

void UDamageHandler::AddIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingDamageModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingDamageModifiers.Remove(Modifier);
	}
}

FCombatModifier UDamageHandler::ModifyDamageTakenFromStat(FDamageInfo const& DamageInfo)
{
	if (IsValid(StatHandler) && StatHandler->GetStatValid(DamageTakenStatTag()))
	{
		return FCombatModifier(StatHandler->GetStatValue(DamageTakenStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UDamageHandler::GetModifiedIncomingDamage(FDamageInfo const& DamageInfo) const
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : IncomingDamageModifiers)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(DamageInfo));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, DamageInfo.Value);
}

void UDamageHandler::AddIncomingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingHealingModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveIncomingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingHealingModifiers.Remove(Modifier);
	}
}

FCombatModifier UDamageHandler::ModifyHealingTakenFromStat(FDamageInfo const& HealingInfo)
{
	if (IsValid(StatHandler) && StatHandler->GetStatValid(HealingTakenStatTag()))
	{
		return FCombatModifier(StatHandler->GetStatValue(HealingTakenStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UDamageHandler::GetModifiedIncomingHealing(FDamageInfo const& HealingInfo) const
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : IncomingHealingModifiers)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(HealingInfo));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, HealingInfo.Value);
}

#pragma endregion