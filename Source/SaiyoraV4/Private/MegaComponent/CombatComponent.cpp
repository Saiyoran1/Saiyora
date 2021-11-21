﻿#include "MegaComponent/CombatComponent.h"
/*#include "Buff.h"
#include "CombatGroup.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameState.h"
#include "UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameStateRef))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in CombatComponent!"));
		return;
	}
}

void UCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	OwnerAsPawn = Cast<APawn>(GetOwner());
	//TODO: Use stat OnRep for client max health initialization?
	//TODO: Add Dot/Hot tags to restricted incoming/outgoing buffs if needed.
	//DAMAGE AND HEALING
	if (!bCanEverReceiveDamage && !bCanEverReceiveHealing)
	{
		bHasHealth = false;
	}
	if (bHasHealth)
	{
		MaxHealth = FMath::Max(DefaultMaxHealth, 1.0f);
		CurrentHealth = MaxHealth;
		LifeStatus = ELifeStatus::Invalid;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, CurrentHealth);
	DOREPLIFETIME(UCombatComponent, LifeStatus);
	DOREPLIFETIME(UCombatComponent, bInCombat);
	DOREPLIFETIME(UCombatComponent, CurrentTarget);
}

bool UCombatComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	return bWroteSomething;
}

#pragma region Damage and Healing

#pragma region Damage

FDamagingEvent UCombatComponent::ApplyDamage(float const Amount, UCombatComponent* AppliedBy, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
	bool const bFromSnapshot, FThreatFromDamage const& ThreatParams)
{
    FDamagingEvent DamageEvent;
    if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !IsValid(Source))
    {
        return DamageEvent;
    }
    if (!bCanEverReceiveDamage || (bHasHealth && LifeStatus != ELifeStatus::Alive))
    {
        return DamageEvent;
    }

    DamageEvent.Info.Value = Amount;
    DamageEvent.Info.SnapshotValue = Amount;
    DamageEvent.Info.AppliedBy = AppliedBy;
    DamageEvent.Info.AppliedTo = this;
    DamageEvent.Info.Source = Source;
    DamageEvent.Info.HitStyle = HitStyle;
    DamageEvent.Info.School = School;
    DamageEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    DamageEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(GetOwner());
    DamageEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        DamageEvent.Info.AppliedByPlane, DamageEvent.Info.AppliedToPlane);

    DamageEvent.ThreatInfo = ThreatParams;
     if (!ThreatParams.SeparateBaseThreat)
     {
         DamageEvent.ThreatInfo.BaseThreat = DamageEvent.Info.Value;
     }

    //Modify the damage, if ignore modifiers is false.
    if (!bIgnoreModifiers)
    {
        //Damage that is not snapshotted should apply outgoing modifiers.
        if (!bFromSnapshot)
        {
            //Apply relevant outgoing mods, save off snapshot damage for use in DoTs.
            DamageEvent.Info.Value = AppliedBy->GetSnapshotDamage(DamageEvent.Info);
            DamageEvent.Info.SnapshotValue = DamageEvent.Info.Value;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
    	for (FDamageModCondition const& Modifier : IncomingDamageModifiers)
    	{
    		if (Modifier.IsBound())
    		{
    			FCombatModifier Mod = Modifier.Execute(DamageEvent.Info);
    			if (Mod.GetModType() != EModifierType::Invalid)
    			{
    				IncomingMods.Add(Mod);
    			}
    		}
    	}
    	//TODO: Add DamageTaken stat mod. Probably condense this to a function for ease of use?
        DamageEvent.Info.Value = FCombatModifier::ApplyModifiers(IncomingMods, DamageEvent.Info.Value);
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions && (CheckIncomingDamageRestricted(DamageEvent.Info) || AppliedBy->CheckOutgoingDamageRestricted(DamageEvent.Info)))
    {
        return DamageEvent;
    }
	
	//TODO: Try enter combat if neither actor is in combat?
	
	DamageEvent.Result.PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageEvent.Info.Value, 0.0f, MaxHealth);
	if (CurrentHealth != DamageEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(DamageEvent.Result.PreviousHealth, CurrentHealth);
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
	AppliedBy->NotifyOfOutgoingDamage(DamageEvent);
	if (DamageEvent.Result.KillingBlow)
	{
		Die();
	}

	//TODO: Add threat?
	
	return DamageEvent;
}

float UCombatComponent::GetSnapshotDamage(float const Amount, UCombatComponent* AppliedTo, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedTo) || !IsValid(Source))
	{
		return 0.0f;
	}
	FDamageInfo Info;
	Info.AppliedBy = this;
	Info.AppliedTo = AppliedTo;
	Info.Source = Source;
	Info.Value = Amount;
	Info.HitStyle = HitStyle;
	Info.School = School;
	Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(GetOwner());
	Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
	Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Info.AppliedByPlane, Info.AppliedToPlane);
	return GetSnapshotDamage(Info);
}

float UCombatComponent::GetSnapshotDamage(FDamageInfo const& Event)
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : OutgoingDamageModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(Event);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				Mods.Add(Mod);
			}
		}
	}
	//TODO: Add DamageDone stat mod.
	return FCombatModifier::ApplyModifiers(Mods, Event.Value);
}

#pragma endregion
#pragma region Healing

FDamagingEvent UCombatComponent::ApplyHealing(float const Amount, UCombatComponent* AppliedBy, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
	bool const bFromSnapshot, FThreatFromDamage const& ThreatParams)
{
    FDamagingEvent HealingEvent;
    if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !IsValid(Source))
    {
        return HealingEvent;
    }
    if (!bCanEverReceiveHealing || (bHasHealth && LifeStatus != ELifeStatus::Alive))
    {
        return HealingEvent;
    }
    HealingEvent.Info.Value = Amount;
    HealingEvent.Info.SnapshotValue = Amount;
    HealingEvent.Info.AppliedBy = AppliedBy;
    HealingEvent.Info.AppliedTo = this;
    HealingEvent.Info.Source = Source;
    HealingEvent.Info.HitStyle = HitStyle;
    HealingEvent.Info.School = School;
    HealingEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    HealingEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(GetOwner());
    HealingEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        HealingEvent.Info.AppliedByPlane, HealingEvent.Info.AppliedToPlane);
    HealingEvent.ThreatInfo = ThreatParams;
    if (!ThreatParams.SeparateBaseThreat)
    {
        HealingEvent.ThreatInfo.BaseThreat = HealingEvent.Info.Value;
    }
    //Modify the healing, if ignore modifiers is false.
    if (!bIgnoreModifiers)
    {
        //Apply relevant outgoing modifiers, save off snapshot healing for use in HoTs.
        if (!bFromSnapshot)
        {
            HealingEvent.Info.Value = AppliedBy->GetSnapshotHealing(HealingEvent.Info);
            HealingEvent.Info.SnapshotValue = HealingEvent.Info.Value;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
        for (FDamageModCondition const& Modifier : IncomingHealingModifiers)
        {
	        if (Modifier.IsBound())
	        {
		        FCombatModifier Mod = Modifier.Execute(HealingEvent.Info);
	        	if (Mod.GetModType() != EModifierType::Invalid)
	        	{
	        		IncomingMods.Add(Mod);
	        	}
	        }
        }
    	//TODO: Add HealingTaken stat mod, probably convert to ModifyIncomingHealing function.
        HealingEvent.Info.Value = FCombatModifier::ApplyModifiers(IncomingMods, HealingEvent.Info.Value);
    }
    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions && (CheckIncomingHealingRestricted(HealingEvent.Info) || AppliedBy->CheckOutgoingHealingRestricted(HealingEvent.Info)))
    {
		return HealingEvent;
    }
	HealingEvent.Result.PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealingEvent.Info.Value, 0.0f, MaxHealth);
	if (CurrentHealth != HealingEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(HealingEvent.Result.PreviousHealth, CurrentHealth);
	}
	//Clear pending killing blow when health gets above 0.
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
	AppliedBy->NotifyOfOutgoingHealing(HealingEvent);
	return HealingEvent;
}

float UCombatComponent::GetSnapshotHealing(float const Amount, UCombatComponent* AppliedTo, UObject* Source,
	EDamageHitStyle const HitStyle, EDamageSchool const School)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedTo) || !IsValid(Source))
	{
		return 0.0f;
	}
	FDamageInfo Info;
	Info.Value = Amount;
	Info.AppliedBy = this;
	Info.AppliedTo = AppliedTo;
	Info.Source = Source;
	Info.HitStyle = HitStyle;
	Info.School = School;
	Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(GetOwner());
	Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
	Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Info.AppliedByPlane, Info.AppliedToPlane);
	return GetSnapshotHealing(Info);
}

float UCombatComponent::GetSnapshotHealing(FDamageInfo const& Event)
{
	TArray<FCombatModifier> Mods;
	for (FDamageModCondition const& Modifier : OutgoingHealingModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(Event);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				Mods.Add(Mod);
			}
		}
	}
	//TODO: Add HealingDone stat mod.
	return FCombatModifier::ApplyModifiers(Mods, Event.Value);
}

#pragma endregion 
#pragma region Subscriptions

void UCombatComponent::SubscribeToHealthChanged(FHealthChangeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnHealthChanged.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromHealthChanged(FHealthChangeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnHealthChanged.Remove(Callback);
}

void UCombatComponent::SubscribeToMaxHealthChanged(FHealthChangeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnMaxHealthChanged.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromMaxHealthChanged(FHealthChangeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnMaxHealthChanged.Remove(Callback);
}

void UCombatComponent::SubscribeToLifeStatusChanged(FLifeStatusCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnLifeStatusChanged.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromLifeStatusChanged(FLifeStatusCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnLifeStatusChanged.Remove(Callback);
}

void UCombatComponent::SubscribeToIncomingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnIncomingDamage.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromIncomingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnIncomingDamage.Remove(Callback);
}

void UCombatComponent::SubscribeToIncomingHealingSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnIncomingHealing.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromIncomingHealingSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnIncomingHealing.Remove(Callback);
}

void UCombatComponent::SubscribeToOutgoingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingDamage.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromOutgoingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingDamage.Remove(Callback);
}

void UCombatComponent::SubscribeToKillingBlow(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnKillingBlow.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromKillingBlow(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnKillingBlow.Remove(Callback);
}

void UCombatComponent::SubscribeToOutgoingHealingSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingHealing.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromOutgoingHealingSuccess(FDamageEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingHealing.Remove(Callback);
}

#pragma endregion
#pragma region Restrictions

void UCombatComponent::AddDeathRestriction(FDeathRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	DeathRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveDeathRestriction(FDeathRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	int32 const Removed = DeathRestrictions.Remove(Restriction);
	if (Removed > 0 && bHasPendingKillingBlow)
	{
		if (!CheckDeathRestricted(PendingKillingBlow))
		{
			Die();
			if (IsValid(PendingKillingBlow.Info.AppliedBy) && PendingKillingBlow.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
			{
				UCombatComponent* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(PendingKillingBlow.Info.AppliedBy);
				if (IsValid(GeneratorComponent))
				{
					GeneratorComponent->NotifyOfDelayedKillingBlow(PendingKillingBlow);
				}
			}
			bHasPendingKillingBlow = false;
			PendingKillingBlow = FDamagingEvent();
		}
	}
}

void UCombatComponent::OnRep_CurrentHealth(float const PreviousHealth)
{
	OnHealthChanged.Broadcast(PreviousHealth, CurrentHealth);
}

void UCombatComponent::OnRep_LifeStatus(ELifeStatus const PreviousStatus)
{
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
}

bool UCombatComponent::CheckDeathRestricted(FDamagingEvent const& DamageEvent)
{
	//TODO: bIgnoreRestrictions for death?
	for (FDeathRestriction const& Restriction : DeathRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(DamageEvent))
		{
			return true;
		}
	}
	return false;
}

void UCombatComponent::AddIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingDamageRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingDamageRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo)
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

void UCombatComponent::AddIncomingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingHealingRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveIncomingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingHealingRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo)
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

void UCombatComponent::AddOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingDamageRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingDamageRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo)
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

void UCombatComponent::AddOutgoingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingHealingRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveOutgoingHealingRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingHealingRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo)
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

#pragma endregion
#pragma region Modifiers

void UCombatComponent::AddIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingDamageModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingDamageModifiers.Remove(Modifier);
}

void UCombatComponent::AddIncomingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingHealingModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveIncomingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingHealingModifiers.Remove(Modifier);
}

void UCombatComponent::AddOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingDamageModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingDamageModifiers.Remove(Modifier);
}

void UCombatComponent::AddOutgoingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingHealingModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveOutgoingHealingModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingHealingModifiers.Remove(Modifier);
}

#pragma endregion
#pragma region Notifies

void UCombatComponent::NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent)
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

void UCombatComponent::NotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfOutgoingHealing(HealingEvent);
	}
}

void UCombatComponent::NotifyOfDelayedKillingBlow(FDamagingEvent const& KillingBlow)
{
	OnKillingBlow.Broadcast(KillingBlow);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfDelayedKillingBlow(KillingBlow);
	}
}

void UCombatComponent::ClientNotifyOfIncomingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnIncomingDamage.Broadcast(DamageEvent);
}

void UCombatComponent::ClientNotifyOfIncomingHealing_Implementation(FDamagingEvent const& HealingEvent)
{
	OnIncomingHealing.Broadcast(HealingEvent);
}

void UCombatComponent::ClientNotifyOfOutgoingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
	if (DamageEvent.Result.KillingBlow)
	{
		OnKillingBlow.Broadcast(DamageEvent);
	}
}

void UCombatComponent::ClientNotifyOfDelayedKillingBlow_Implementation(FDamagingEvent const& KillingBlow)
{
	OnKillingBlow.Broadcast(KillingBlow);
}

void UCombatComponent::ClientNotifyOfOutgoingHealing_Implementation(FDamagingEvent const& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
}

#pragma endregion

void UCombatComponent::Die()
{
	ELifeStatus const PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Dead;
	if (LifeStatus != PreviousStatus)
	{
		OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
	}
	//TODO: Clear threat table, clear actor from all other threat tables.
	//TODO: Remove all buffs that don't persist through death.
	//TODO: Cancel cast if necessary.
}

#pragma endregion
#pragma region Threat

float UCombatComponent::GlobalHealingThreatModifier = 0.3f;
float UCombatComponent::GlobalTauntThreatPercentage = 1.2f;
float UCombatComponent::GlobalThreatDecayPercentage = 0.9f;
float UCombatComponent::GlobalThreatDecayInterval = 3.0f;

FThreatEvent UCombatComponent::AddThreat(EThreatType const ThreatType, float const BaseThreat, UCombatComponent* AppliedBy,
	UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
	FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
    	
    if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || !IsValid(AppliedBy) || BaseThreat <= 0.0f)
    {
    	return Result;
    }
    	
    //Applied by and target must both either be alive or not have health.
    if ((bHasHealth && LifeStatus != ELifeStatus::Alive) || (AppliedBy->HasHealth() && AppliedBy->GetLifeStatus() != ELifeStatus::Alive))
    {
    	return Result;
    }

    if (!IsValid(AppliedBy->GetMisdirectTarget()) || (AppliedBy->GetMisdirectTarget()->HasHealth() && AppliedBy->GetMisdirectTarget()->GetLifeStatus() != ELifeStatus::Alive))
    {
    	Result.AppliedBy = AppliedBy;
    }
    else
    {
    	Result.AppliedBy = AppliedBy->GetMisdirectTarget();
    }
    
    Result.AppliedTo = this;
   	Result.Source = Source;
    Result.ThreatType = ThreatType;
   	Result.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedBy);
    Result.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedTo);
    Result.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
    Result.Threat = BaseThreat;
    
    if (!bIgnoreModifiers)
    {
    	TArray<FCombatModifier> Mods;
    	//TODO: ModifyOutgoingThreat function that takes into account the SourceModifier.
    	AppliedBy->GetOutgoingThreatMods(Result, Mods);
    	if (SourceModifier.IsBound())
    	{
    		Mods.Add(SourceModifier.Execute(Result));
    	}
    	//Apply outgoing (and source) mods first to emulate the damage calculation.
    	Result.Threat = FCombatModifier::ApplyModifiers(Mods, Result.Threat);
    	Mods.Empty();
    	GetIncomingThreatMods(Result, Mods);
    	//Apply incoming mods afterwards.
    	Result.Threat = FCombatModifier::ApplyModifiers(Mods, Result.Threat);
    	if (Result.Threat <= 0.0f)
    	{
    		return Result;
    	}
    }
    
    if (!bIgnoreRestrictions && (AppliedBy->CheckOutgoingThreatRestricted(Result) || CheckIncomingThreatRestricted(Result)))
    {
    	//Possible fuckery with misdirect target restricting threat being bypassed, since we just check the original AppliedBy here?
    	return Result;
    }
    
    int32 FoundIndex = ThreatTable.Num();
    bool bFound = false;
	//I don't know why I used reverse iteration, maybe I'm a moron.
    for (; FoundIndex > 0; FoundIndex--)
    {
    	if (ThreatTable[FoundIndex - 1].Target == Result.AppliedBy)
    	{
    		bFound = true;
    		ThreatTable[FoundIndex - 1].Threat += Result.Threat;
    		//As a note, the sorting magic works because the < operator is overloaded for FThreatTarget.
    		//This means it takes into account Fixates, Fades, and Blinds.
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
    	AddToThreatTable(Result.AppliedBy, Result.Threat, AppliedBy->HasActiveFade());
    }
    Result.bSuccess = true;
    
    return Result;
}

void UCombatComponent::RemoveThreat(UCombatComponent* Target, float const Amount)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || !IsValid(Target))
	{
		return;
	}
	bool bAffectedTarget = false;
	bool bFound = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bFound = true;
			ThreatTable[i].Threat = FMath::Max(0.0f, ThreatTable[i].Threat - Amount);
			//As a note, the sorting magic works because the < operator is overloaded for FThreatTarget.
			//This means it takes into account Fixates, Fades, and Blinds.
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

void UCombatComponent::AddToThreatTable(UCombatComponent* Target, float const InitialThreat, bool const bFaded,
                                        UBuff* InitialFixate, UBuff* InitialBlind)
{
	if (GetOwnerRole() != ROLE_Authority || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(Target) || (Target->HasHealth() && Target->GetLifeStatus() != ELifeStatus::Alive))
    {
    	return;
    }
    if (ThreatTable.Num() == 0)
    {
    	ThreatTable.Add(FThreatTarget(Target, InitialThreat, bFaded, InitialFixate, InitialBlind));
    	UpdateTarget();
    	UpdateCombatStatus();
    }
    else
    {
    	for (FThreatTarget const& ThreatTarget : ThreatTable)
    	{
    		if (ThreatTarget.Target == Target)
    		{
    			return;
    		}
    	}
    	FThreatTarget const NewTarget = FThreatTarget(Target, InitialThreat, bFaded, InitialFixate, InitialBlind);
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
	Target->NotifyAddedToThreatTable(this);
}

void UCombatComponent::RemoveFromThreatTable(UCombatComponent* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target))
	{
		return;
	}
    bool bAffectedTarget = false;
    for (int i = 0; i < ThreatTable.Num(); i++)
    {
    	if (ThreatTable[i].Target == Target)
    	{
    		if (i == ThreatTable.Num() - 1)
    		{
    			bAffectedTarget = true;
    		}
    		ThreatTable.RemoveAt(i);
    		Target->NotifyRemovedFromThreatTable(this);
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

void UCombatComponent::NotifyAddedToThreatTable(UCombatComponent* Target)
{
	if (!IsValid(Target) || TargetedBy.Contains(Target))
	{
		return;
	}
	TargetedBy.Add(Target);
	if (TargetedBy.Num() == 1)
	{
		UpdateCombatStatus();
	}
}

void UCombatComponent::NotifyRemovedFromThreatTable(UCombatComponent* Target)
{
	if (!IsValid(Target))
	{
		return;
	}
	if (TargetedBy.Remove(Target) > 0 && TargetedBy.Num() == 0)
	{
		UpdateCombatStatus();
	}
}

void UCombatComponent::UpdateCombatStatus()
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
			OnCombatStatusChanged.Broadcast(false);
			GetWorld()->GetTimerManager().ClearTimer(DecayHandle);
		}
	}
	else
	{
		if (ThreatTable.Num() > 0 || TargetedBy.Num() > 0)
		{
			bInCombat = true;
			OnCombatStatusChanged.Broadcast(true);
			GetWorld()->GetTimerManager().SetTimer(DecayHandle, DecayDelegate, GlobalThreatDecayInterval, true);
		}
	}
}

void UCombatComponent::OnRep_bInCombat()
{
	OnCombatStatusChanged.Broadcast(bInCombat);
}

void UCombatComponent::UpdateTarget()
{
	UCombatComponent* Previous = CurrentTarget;
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

void UCombatComponent::OnRep_CurrentTarget(UCombatComponent* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

float UCombatComponent::GetThreatLevel(UCombatComponent* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target))
	{
		return 0.0f;
	}
	for (FThreatTarget const& ThreatTarget : ThreatTable)
	{
		if (ThreatTarget.Target == Target)
		{
			return ThreatTarget.Threat;
		}
	}
	return 0.0f;
}

void UCombatComponent::DecayThreat()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (FThreatTarget& Target : ThreatTable)
		{
			Target.Threat *= GlobalThreatDecayPercentage;
		}
	}
}

#pragma region Threat Special Events

void UCombatComponent::Taunt(UCombatComponent* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(AppliedBy) || (AppliedBy->HasHealth() && AppliedBy->GetLifeStatus() != ELifeStatus::Alive))
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

void UCombatComponent::DropThreat(UCombatComponent* Target, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || !IsValid(Target))
	{
		return;
	}
	float const DropThreat = GetThreatLevel(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(Target, DropThreat);
}

void UCombatComponent::TransferThreat(UCombatComponent* From, UCombatComponent* To, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive))
	{
		return;
	}
	if (!IsValid(From) || (From->HasHealth() && From->GetLifeStatus() != ELifeStatus::Alive) || !IsValid(To) || (To->HasHealth() && To->GetLifeStatus() != ELifeStatus::Alive))
	{
		return;
	}
	float const TransferThreat = GetThreatLevel(From) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(From, TransferThreat);
	AddThreat(EThreatType::Absolute, TransferThreat, To, nullptr, true, true, FThreatModCondition());
}

void UCombatComponent::Vanish()
{
	//TODO: Do this through combat group?
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	for (UCombatComponent* Target : TargetedBy)
	{
		if (IsValid(Target))
		{
			Target->NotifyOfTargetVanish(this);
		}
	}
	//This only clears us from actors that have us in their threat table. This does NOT clear our threat table. This shouldn't matter, as if an NPC vanishes its unlikely the player will care.
	//This only has ramifications if NPCs can possibly attack each other.
}

void UCombatComponent::AddFixate(UCombatComponent* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!IsValid(Target) || (Target->HasHealth() && Target->GetLifeStatus() != ELifeStatus::Alive))
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
		AddToThreatTable(Target, 0.0f, Target->HasActiveFade(), Source);
	}
}

void UCombatComponent::RemoveFixate(UCombatComponent* Target, UBuff* Source)
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

void UCombatComponent::AddBlind(UCombatComponent* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!IsValid(Target) || (Target->HasHealth() && Target->GetLifeStatus() != ELifeStatus::Alive))
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
		AddToThreatTable(Target, 0.0f, Target->HasActiveFade(), nullptr, Source);
	}
}

void UCombatComponent::RemoveBlind(UCombatComponent* Target, UBuff* Source)
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

void UCombatComponent::AddFade(UBuff* Source)
{
	//TODO: Do this through combat group?
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (Fades.Contains(Source))
	{
		return;
	}
	int32 const PreviousFades = Fades.Num();
	Fades.Add(Source);
	if (PreviousFades == 0 && Fades.Num() == 1)
	{
		for (UCombatComponent* Target : TargetedBy)
		{
			if (IsValid(Target))
			{
				Target->NotifyOfTargetFadeStatusChange(this, true);
			}
		}
	}
}

void UCombatComponent::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || Fades.Num() == 0)
	{
		return;
	}
	int32 const Removed = Fades.Remove(Source);
	if (Removed != 0 && Fades.Num() == 0)
	{
		for (UCombatComponent* Target : TargetedBy)
		{
			if (IsValid(Target))
			{
				Target->NotifyOfTargetFadeStatusChange(this, false);
			}
		}
	}
}

void UCombatComponent::NotifyOfTargetFadeStatusChange(UCombatComponent* Target, bool const bFaded)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target))
    {
    	return;
    }
    for (int i = 0; i < ThreatTable.Num(); i++)
    {
    	if (ThreatTable[i].Target == Target)
    	{
    		bool AffectedTarget = false;
    		ThreatTable[i].Faded = bFaded;
    		if (bFaded)
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

void UCombatComponent::AddMisdirect(UBuff* Source, UCombatComponent* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || (bHasHealth && LifeStatus != ELifeStatus::Alive) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (!IsValid(Target) || (Target->HasHealth() && Target->GetLifeStatus() != ELifeStatus::Alive))
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

void UCombatComponent::RemoveMisdirect(UBuff* Source)
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
#pragma region Subscriptions

void UCombatComponent::SubscribeToTargetChanged(FTargetCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnTargetChanged.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromTargetChanged(FTargetCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnTargetChanged.Remove(Callback);
}

void UCombatComponent::SubscribeToCombatStatusChanged(FCombatStatusCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCombatStatusChanged.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromCombatStatusChanged(FCombatStatusCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCombatStatusChanged.Remove(Callback);
}

#pragma endregion
#pragma region Restrictions

void UCombatComponent::AddIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingThreatRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingThreatRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckIncomingThreatRestricted(FThreatEvent const& ThreatEvent)
{
	for (FThreatRestriction const& Restriction : IncomingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(ThreatEvent))
		{
			return true;
		}
	}
	return false;
}

void UCombatComponent::AddOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingThreatRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingThreatRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckOutgoingThreatRestricted(FThreatEvent const& ThreatEvent)
{
	for (FThreatRestriction const& Restriction : OutgoingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(ThreatEvent))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UCombatComponent::AddIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingThreatMods.AddUnique(Modifier);
}

void UCombatComponent::RemoveIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingThreatMods.Remove(Modifier);
}

void UCombatComponent::GetIncomingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods)
{
	for (FThreatModCondition const& Modifier : IncomingThreatMods)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(ThreatEvent);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
}

void UCombatComponent::AddOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingThreatMods.AddUnique(Modifier);
}

void UCombatComponent::RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingThreatMods.Remove(Modifier);
}

void UCombatComponent::GetOutgoingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods)
{
	for (FThreatModCondition const& Modifier : OutgoingThreatMods)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(ThreatEvent);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
}

#pragma endregion 

#pragma endregion*/