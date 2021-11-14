#include "MegaComponent/CombatComponent.h"
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
}

bool UCombatComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	return bWroteSomething;
}

#pragma region Damage and Healing

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

void UCombatComponent::SubscribeToIncomingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnIncomingHealing.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromIncomingHealingSuccess(FHealingEventCallback const& Callback)
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

void UCombatComponent::SubscribeToOutgoingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingHealing.AddUnique(Callback);
}

void UCombatComponent::UnsubscribeFromOutgoingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnOutgoingHealing.Remove(Callback);
}

#pragma endregion
#pragma region Restrictions

void UCombatComponent::AddDeathRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	DeathRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveDeathRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	int32 const Removed = DeathRestrictions.Remove(Restriction);
	if (Removed > 0)
	{
		//TODO: Recheck for pending death that was previously restricted.
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

bool UCombatComponent::CheckDeathRestricted(FDamageInfo const& DamageInfo)
{
	//TODO: Change this to a bIgnoreRestrictions flag in the struct.
	if (DamageInfo.HitStyle == EDamageHitStyle::Authority)
	{
		return false;
	}
	for (FDamageRestriction const& Restriction : DeathRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(DamageInfo))
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

void UCombatComponent::AddIncomingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingHealingRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveIncomingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	IncomingHealingRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckIncomingHealingRestricted(FHealingInfo const& HealingInfo)
{
	for (FHealingRestriction const& Restriction : IncomingHealingRestrictions)
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

void UCombatComponent::AddOutgoingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingHealingRestrictions.AddUnique(Restriction);
}

void UCombatComponent::RemoveOutgoingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	OutgoingHealingRestrictions.Remove(Restriction);
}

bool UCombatComponent::CheckOutgoingHealingRestricted(FHealingInfo const& HealingInfo)
{
	for (FHealingRestriction const& Restriction : OutgoingHealingRestrictions)
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

void UCombatComponent::GetIncomingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods)
{
	for (FDamageModCondition const& Modifier : IncomingDamageModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(DamageInfo);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
	//TODO: Add DamageTaken stat mod.
}

void UCombatComponent::AddIncomingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingHealingModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveIncomingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	IncomingHealingModifiers.Remove(Modifier);
}

void UCombatComponent::GetIncomingHealingMods(FHealingInfo const& HealingInfo, TArray<FCombatModifier>& OutMods)
{
	for (FHealingModCondition const& Modifier : IncomingHealingModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(HealingInfo);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
	//TODO: Add HealingTaken stat mod.
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

void UCombatComponent::GetOutgoingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods)
{
	for (FDamageModCondition const& Modifier : OutgoingDamageModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(DamageInfo);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
	//TODO: Add DamageDone stat mod.
}

void UCombatComponent::AddOutgoingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingHealingModifiers.AddUnique(Modifier);
}

void UCombatComponent::RemoveOutgoingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return;
	}
	OutgoingHealingModifiers.Remove(Modifier);
}

void UCombatComponent::GetOutgoingHealingMods(FHealingInfo const& HealingInfo, TArray<FCombatModifier>& OutMods)
{
	for (FHealingModCondition const& Modifier : OutgoingHealingModifiers)
	{
		if (Modifier.IsBound())
		{
			FCombatModifier Mod = Modifier.Execute(HealingInfo);
			if (Mod.GetModType() != EModifierType::Invalid)
			{
				OutMods.Add(Mod);
			}
		}
	}
	//TODO: Add HealingDone stat mod.
}

#pragma endregion
#pragma region Notifies

void UCombatComponent::NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfOutgoingDamage(DamageEvent);
	}
}

void UCombatComponent::NotifyOfOutgoingHealing(FHealingEvent const& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
	if (IsValid(OwnerAsPawn) && !OwnerAsPawn->IsLocallyControlled())
	{
		ClientNotifyOfOutgoingHealing(HealingEvent);
	}
}

void UCombatComponent::ClientNotifyOfIncomingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnIncomingDamage.Broadcast(DamageEvent);
}

void UCombatComponent::ClientNotifyOfIncomingHealing_Implementation(FHealingEvent const& HealingEvent)
{
	OnIncomingHealing.Broadcast(HealingEvent);
}

void UCombatComponent::ClientNotifyOfOutgoingDamage_Implementation(FDamagingEvent const& DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
}

void UCombatComponent::ClientNotifyOfOutgoingHealing_Implementation(FHealingEvent const& HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
}

#pragma endregion

void UCombatComponent::ApplyDamage(FDamagingEvent& DamageEvent)
{
	DamageEvent.Result.PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - DamageEvent.DamageInfo.Damage, 0.0f, MaxHealth);
	if (CurrentHealth != DamageEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(DamageEvent.Result.PreviousHealth, CurrentHealth);
	}
    DamageEvent.Result.Success = true;
    DamageEvent.Result.NewHealth = CurrentHealth;
    DamageEvent.Result.AmountDealt = DamageEvent.Result.PreviousHealth - CurrentHealth;
    if (CurrentHealth == 0.0f && !CheckDeathRestricted(DamageEvent.DamageInfo))
    {
    	//TODO: Save pending killing blow.
    	DamageEvent.Result.KillingBlow = true;
    }
    ClientNotifyOfIncomingDamage(DamageEvent);
    if (DamageEvent.Result.KillingBlow)
    {
    	Die();
    }
}

void UCombatComponent::ApplyHealing(FHealingEvent& HealingEvent)
{
	HealingEvent.Result.PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + HealingEvent.HealingInfo.Healing, 0.0f, MaxHealth);
	if (CurrentHealth != HealingEvent.Result.PreviousHealth)
	{
		OnHealthChanged.Broadcast(HealingEvent.Result.PreviousHealth, CurrentHealth);
	}
	//TODO: Check if previously pending death, clear pending killing blow if health increases above zero.
    HealingEvent.Result.Success = true;
    HealingEvent.Result.NewHealth = CurrentHealth;
    HealingEvent.Result.AmountDealt = CurrentHealth - HealingEvent.Result.PreviousHealth;
    ClientNotifyOfIncomingHealing(HealingEvent);	
}

void UCombatComponent::Die()
{
	//TODO: Convert to generic SetLifeStatus() function that can be reused during respawn?
	ELifeStatus const PreviousStatus = LifeStatus;
    LifeStatus = ELifeStatus::Dead;
	if (LifeStatus != PreviousStatus)
	{
		OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
	}
}

#pragma endregion