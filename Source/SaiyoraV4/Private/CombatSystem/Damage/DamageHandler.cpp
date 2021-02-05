// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageHandler.h"

#include "SaiyoraStatLibrary.h"
#include "StatHandler.h"
#include "BuffHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "UnrealNetwork.h"

FGameplayTag const UDamageHandler::MaxHealthTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxHealth")), false);
FGameplayTag const UDamageHandler::DamageDoneTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageDone")), false);
FGameplayTag const UDamageHandler::HealingDoneTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingDone")), false);
FGameplayTag const UDamageHandler::DamageTakenTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageTaken")), false);
FGameplayTag const UDamageHandler::HealingTakenTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingTaken")), false);
FGameplayTag const UDamageHandler::DamageBuffTag = FGameplayTag::RequestGameplayTag(FName(TEXT("BuffFunction.Damage")), false);
FGameplayTag const UDamageHandler::HealingBuffTag = FGameplayTag::RequestGameplayTag(FName(TEXT("BuffFunction.Healing")), false);

UDamageHandler::UDamageHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UDamageHandler::InitializeComponent()
{
	MaxHealth = DefaultMaxHealth;
	CurrentHealth = MaxHealth;
	LifeStatus = ELifeStatus::Invalid;
}

void UDamageHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Bind Max Health stat, create damage and healing modifiers from stats.
		UStatHandler* StatHandler = GetOwner()->FindComponentByClass<UStatHandler>();
		if (StatHandler)
		{
			if (!StaticMaxHealth && StatHandler->GetStatValid(MaxHealthTag))
			{
				UpdateMaxHealth(StatHandler->GetStatValue(MaxHealthTag));

				FStatCallback MaxHealthStatCallback;
				MaxHealthStatCallback.BindUFunction(this, FName(TEXT("ReactToMaxHealthStat")));
				StatHandler->SubscribeToStatChanged(MaxHealthTag, MaxHealthStatCallback);
			}
			if (StatHandler->GetStatValid(DamageDoneTag))
			{
				FDamageModCondition DamageDoneModFromStat;
				DamageDoneModFromStat.BindUFunction(this, FName(TEXT("ModifyDamageDoneFromStat")));
				AddOutgoingDamageModifier(DamageDoneModFromStat);
			}
			if (StatHandler->GetStatValid(DamageTakenTag))
			{
				FDamageModCondition DamageTakenModFromStat;
				DamageTakenModFromStat.BindUFunction(this, FName(TEXT("ModifyDamageTakenFromStat")));
				AddIncomingDamageModifier(DamageTakenModFromStat);
			}
			if (StatHandler->GetStatValid(HealingDoneTag))
			{
				FHealingModCondition HealingDoneModFromStat;
				HealingDoneModFromStat.BindUFunction(this, FName(TEXT("ModifyHealingDoneFromStat")));
				AddOutgoingHealingModifier(HealingDoneModFromStat);
			}
			if (StatHandler->GetStatValid(HealingTakenTag))
			{
				FHealingModCondition HealingTakenModFromStat;
				HealingTakenModFromStat.BindUFunction(this, FName(TEXT("ModifyHealingTakenFromStat")));
				AddIncomingHealingModifier(HealingTakenModFromStat);
			}
		}

		//Add buff restrictions for damage and healing interactions that are not enabled.
		UBuffHandler* BuffHandler = USaiyoraBuffLibrary::GetBuffHandler(GetOwner());
		if (BuffHandler)
		{
			if (!CanEverDealDamage())
			{
				FBuffEventCondition DamageBuffRestriction;
				DamageBuffRestriction.BindUFunction(this, FName("RestrictOutgoingDamageBuffs"));
				BuffHandler->AddOutgoingBuffRestriction(DamageBuffRestriction);
			}

			if (!CanEverReceiveDamage())
			{
				FBuffEventCondition DamageBuffRestriction;
				DamageBuffRestriction.BindUFunction(this, FName("RestrictIncomingDamageBuffs"));
				BuffHandler->AddIncomingBuffRestriction(DamageBuffRestriction);
			}

			if (!CanEverDealHealing())
			{
				FBuffEventCondition HealingBuffRestriction;
				HealingBuffRestriction.BindUFunction(this, FName("RestrictOutgoingHealingBuffs"));
				BuffHandler->AddOutgoingBuffRestriction(HealingBuffRestriction);
			}

			if (!CanEverReceiveHealing())
			{
				FBuffEventCondition HealingBuffRestriction;
				HealingBuffRestriction.BindUFunction(this, FName("RestrictIncomingHealingBuffs"));
				BuffHandler->AddIncomingBuffRestriction(HealingBuffRestriction);
			}
		}
		LifeStatus = ELifeStatus::Alive;
	}
}

//Health

void UDamageHandler::UpdateMaxHealth(float const NewMaxHealth)
{
	float const PreviousMaxHealth = MaxHealth;
	if (StaticMaxHealth)
	{
		MaxHealth = DefaultMaxHealth;
	}
	else
	{
		if (NewMaxHealth >= 0.0f)
		{
			MaxHealth = FMath::Max(1.0f, NewMaxHealth);
			if (HealthFollowsMaxHealth)
			{
				float const PreviousHealth = CurrentHealth;
				CurrentHealth = (CurrentHealth / PreviousMaxHealth) * MaxHealth;
				if (CurrentHealth != PreviousHealth)
				{
					OnHealthChanged.Broadcast(PreviousHealth, CurrentHealth);
				}
			}
		}
	}
	if (MaxHealth != PreviousMaxHealth)
	{
		OnMaxHealthChanged.Broadcast(PreviousMaxHealth, MaxHealth);
	}
}

void UDamageHandler::ReactToMaxHealthStat(FGameplayTag const& StatTag, float const NewValue)
{
	if (StatTag.IsValid() && StatTag.MatchesTag(MaxHealthTag))
	{
		UpdateMaxHealth(NewValue);
	}
}


bool UDamageHandler::CheckDeathRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageCondition Condition : DeathConditions)
	{
		if (Condition.IsBound() && Condition.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

void UDamageHandler::Die()
{
	ELifeStatus const PreviousStatus = LifeStatus;
	LifeStatus = ELifeStatus::Dead;
	OnLifeStatusChanged.Broadcast(PreviousStatus, LifeStatus);
}

void UDamageHandler::OnRep_CurrentHealth(float PreviousValue)
{
	OnHealthChanged.Broadcast(PreviousValue, CurrentHealth);
}

void UDamageHandler::OnRep_MaxHealth(float PreviousValue)
{
	OnMaxHealthChanged.Broadcast(PreviousValue, MaxHealth);
}

void UDamageHandler::OnRep_LifeStatus(ELifeStatus PreviousValue)
{
	OnLifeStatusChanged.Broadcast(PreviousValue, LifeStatus);
}

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

void UDamageHandler::AddDeathRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		DeathConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveDeathRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		DeathConditions.RemoveSingleSwap(Restriction);
	}
}

//Outgoing Damage

void UDamageHandler::NotifyOfOutgoingDamageSuccess(FDamagingEvent const& DamageEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		MulticastNotifyOfOutgoingDamageSuccess(DamageEvent);
	}
}

bool UDamageHandler::CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageCondition Condition : OutgoingDamageConditions)
	{
		if (Condition.IsBound() && Condition.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

FCombatModifier UDamageHandler::ModifyDamageDoneFromStat(FDamageInfo const& DamageInfo)
{
	FCombatModifier Mod;
	Mod.ModifierType = EModifierType::Multiplicative;
	UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(GetOwner());
	if (!StatHandler)
	{
		Mod.ModifierType = EModifierType::Invalid;
		return Mod;
	}
	Mod.ModifierValue = StatHandler->GetStatValue(DamageDoneTag);
	if (Mod.ModifierValue < 0.0f)
	{
		Mod.ModifierType = EModifierType::Invalid;
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetRelevantOutgoingDamageMods(FDamageInfo const& DamageInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (FDamageModCondition const& Mod : OutgoingDamageModifiers)
	{
		FCombatModifier TempMod = Mod.Execute(DamageInfo);
		if (TempMod.ModifierType != EModifierType::Invalid)
		{
			RelevantMods.Add(TempMod);
		}
	}
	return RelevantMods;
}

void UDamageHandler::SubscribeToOutgoingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingDamage.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromOutgoingDamageSuccess(FDamageEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingDamage.Remove(Callback);
	}
}

void UDamageHandler::AddOutgoingDamageRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		OutgoingDamageConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingDamageRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		OutgoingDamageConditions.RemoveSingleSwap(Restriction);
	}
}

void UDamageHandler::AddOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		OutgoingDamageModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		OutgoingDamageModifiers.RemoveSingleSwap(Modifier);
	}
}

void UDamageHandler::MulticastNotifyOfOutgoingDamageSuccess_Implementation(FDamagingEvent DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
}

//Outgoing Healing

void UDamageHandler::NotifyOfOutgoingHealingSuccess(FHealingEvent const& HealingEvent)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		MulticastNotifyOfOutgoingHealingSuccess(HealingEvent);
	}
}

bool UDamageHandler::CheckOutgoingHealingRestricted(FHealingInfo const& HealingInfo)
{
	for (FHealingCondition const& Condition : OutgoingHealingConditions)
	{
		if (Condition.IsBound() && Condition.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

FCombatModifier UDamageHandler::ModifyHealingDoneFromStat(FHealingInfo const& HealingInfo)
{
	FCombatModifier Mod;
	Mod.ModifierType = EModifierType::Multiplicative;
	UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(GetOwner());
	if (!StatHandler)
	{
		Mod.ModifierType = EModifierType::Invalid;
		return Mod;
	}
	Mod.ModifierValue = StatHandler->GetStatValue(HealingDoneTag);
	if (Mod.ModifierValue < 0.0f)
	{
		Mod.ModifierType = EModifierType::Invalid;
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetRelevantOutgoingHealingMods(FHealingInfo const& HealingInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (FHealingModCondition const& Mod : OutgoingHealingModifiers)
	{
		FCombatModifier TempMod = Mod.Execute(HealingInfo);
		if (TempMod.ModifierType != EModifierType::Invalid)
		{
			RelevantMods.Add(TempMod);
		}
	}
	return RelevantMods;
}

void UDamageHandler::SubscribeToOutgoingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingHealing.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromOutgoingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnOutgoingHealing.Remove(Callback);
	}
}

void UDamageHandler::AddOutgoingHealingRestriction(FHealingCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		OutgoingHealingConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingHealingRestriction(FHealingCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		OutgoingHealingConditions.RemoveSingleSwap(Restriction);
	}
}

void UDamageHandler::AddOutgoingHealingModifier(FHealingModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		OutgoingHealingModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveOutgoingHealingModifier(FHealingModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		OutgoingHealingModifiers.RemoveSingleSwap(Modifier);
	}
}

void UDamageHandler::MulticastNotifyOfOutgoingHealingSuccess_Implementation(FHealingEvent HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
}

//Incoming Damage

void UDamageHandler::ApplyDamage(FDamagingEvent& DamageEvent)
{
	DamageEvent.Result.PreviousHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageEvent.DamageInfo.Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(DamageEvent.Result.PreviousHealth, CurrentHealth);
	
	DamageEvent.Result.Success = true;
	DamageEvent.Result.NewHealth = CurrentHealth;
	DamageEvent.Result.AmountDealt = DamageEvent.Result.PreviousHealth - CurrentHealth;
	if (CurrentHealth == 0.0f && !CheckDeathRestricted(DamageEvent.DamageInfo))
	{
		DamageEvent.Result.KillingBlow = true;
	}
	
	MulticastNotifyOfIncomingDamageSuccess(DamageEvent);

	if (DamageEvent.Result.KillingBlow)
	{
		Die();
	}
}

bool UDamageHandler::CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageCondition const& Condition : IncomingDamageConditions)
	{
		if (Condition.IsBound() && Condition.Execute(DamageInfo))
		{
			return true;
		}
	}
	return false;
}

FCombatModifier UDamageHandler::ModifyDamageTakenFromStat(FDamageInfo const& DamageInfo)
{
	FCombatModifier Mod;
	Mod.ModifierType = EModifierType::Multiplicative;
	UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(GetOwner());
	if (!StatHandler)
	{
		Mod.ModifierType = EModifierType::Invalid;
		return Mod;
	}
	Mod.ModifierValue = StatHandler->GetStatValue(DamageTakenTag);
	if (Mod.ModifierValue < 0.0f)
	{
		Mod.ModifierType = EModifierType::Invalid;
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetRelevantIncomingDamageMods(FDamageInfo const& DamageInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (FDamageModCondition const& Mod : IncomingDamageModifiers)
	{
		if (Mod.IsBound())
		{
			FCombatModifier TempMod = Mod.Execute(DamageInfo);
			if (TempMod.ModifierType != EModifierType::Invalid)
			{
				RelevantMods.Add(TempMod);
			}
		}
	}
	return RelevantMods;
}

void UDamageHandler::SubscribeToIncomingDamageSuccess(FDamageEventCallback const& Callback)
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

void UDamageHandler::AddIncomingDamageRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		IncomingDamageConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingDamageRestriction(FDamageCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		IncomingDamageConditions.RemoveSingleSwap(Restriction);
	}
}

void UDamageHandler::AddIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		IncomingDamageModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		IncomingDamageModifiers.RemoveSingleSwap(Modifier);
	}
}

void UDamageHandler::MulticastNotifyOfIncomingDamageSuccess_Implementation(FDamagingEvent DamageEvent)
{
	OnIncomingDamage.Broadcast(DamageEvent);
}

//Incoming Healing

void UDamageHandler::ApplyHealing(FHealingEvent& HealingEvent)
{
	HealingEvent.Result.PreviousHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(CurrentHealth + HealingEvent.HealingInfo.Healing, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(HealingEvent.Result.PreviousHealth, CurrentHealth);
	
	HealingEvent.Result.Success = true;
	HealingEvent.Result.NewHealth = CurrentHealth;
	HealingEvent.Result.AmountDealt = CurrentHealth - HealingEvent.Result.PreviousHealth;
	
	MulticastNotifyOfIncomingHealingSuccess(HealingEvent);
}

bool UDamageHandler::CheckIncomingHealingRestricted(FHealingInfo const& HealingInfo)
{
	for (FHealingCondition const& Condition : IncomingHealingConditions)
	{
		if (Condition.IsBound() && Condition.Execute(HealingInfo))
		{
			return true;
		}
	}
	return false;
}

FCombatModifier UDamageHandler::ModifyHealingTakenFromStat(FHealingInfo const& HealingInfo)
{
	FCombatModifier Mod;
	Mod.ModifierType = EModifierType::Multiplicative;
	UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(GetOwner());
	if (!StatHandler)
	{
		Mod.ModifierType = EModifierType::Invalid;
		return Mod;
	}
	Mod.ModifierValue = StatHandler->GetStatValue(HealingTakenTag);
	if (Mod.ModifierValue < 0.0f)
	{
		Mod.ModifierType = EModifierType::Invalid;
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetRelevantIncomingHealingMods(FHealingInfo const& HealingInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (FHealingModCondition const& Mod : IncomingHealingModifiers)
	{
		if (Mod.IsBound())
		{
			FCombatModifier TempMod = Mod.Execute(HealingInfo);
			if (TempMod.ModifierType != EModifierType::Invalid)
			{
				RelevantMods.Add(TempMod);
			}
		}
	}
	return RelevantMods;
}

void UDamageHandler::SubscribeToIncomingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingHealing.AddUnique(Callback);
	}
}

void UDamageHandler::UnsubscribeFromIncomingHealingSuccess(FHealingEventCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnIncomingHealing.Remove(Callback);
	}
}

void UDamageHandler::AddIncomingHealingRestriction(FHealingCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		IncomingHealingConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingHealingRestriction(FHealingCondition const& Restriction)
{
	if (Restriction.IsBound())
	{
		IncomingHealingConditions.RemoveSingleSwap(Restriction);
	}
}

void UDamageHandler::AddIncomingHealingModifier(FHealingModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		IncomingHealingModifiers.AddUnique(Modifier);
	}
}

void UDamageHandler::RemoveIncomingHealingModifier(FHealingModCondition const& Modifier)
{
	if (Modifier.IsBound())
	{
		IncomingHealingModifiers.RemoveSingleSwap(Modifier);
	}
}

void UDamageHandler::MulticastNotifyOfIncomingHealingSuccess_Implementation(FHealingEvent HealingEvent)
{
	OnIncomingHealing.Broadcast(HealingEvent);
}

//Replication

void UDamageHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDamageHandler, CurrentHealth);
	DOREPLIFETIME(UDamageHandler, MaxHealth);
	DOREPLIFETIME(UDamageHandler, LifeStatus);
}
