#include "DamageHandler.h"
#include "StatHandler.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"

/*UDamageHandler::UDamageHandler()
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
	
	if (IsValid(GetOwner()) && GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		StatHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
		BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
	}
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Bind Max Health stat, create damage and healing modifiers from stats.
		if (IsValid(StatHandler))
		{
			if (!StaticMaxHealth && StatHandler->GetStatValid(MaxHealthStatTag()))
			{
				UpdateMaxHealth(StatHandler->GetStatValue(MaxHealthStatTag()));

				FStatCallback MaxHealthStatCallback;
				MaxHealthStatCallback.BindDynamic(this, &UDamageHandler::ReactToMaxHealthStat);
				StatHandler->SubscribeToStatChanged(MaxHealthStatTag(), MaxHealthStatCallback);
			}
			if (StatHandler->GetStatValid(DamageDoneStatTag()))
			{
				FDamageModCondition DamageDoneModFromStat;
				DamageDoneModFromStat.BindDynamic(this, &UDamageHandler::ModifyDamageDoneFromStat);
				AddOutgoingDamageModifier(DamageDoneModFromStat);
			}
			if (StatHandler->GetStatValid(DamageTakenStatTag()))
			{
				FDamageModCondition DamageTakenModFromStat;
				DamageTakenModFromStat.BindDynamic(this, &UDamageHandler::ModifyDamageTakenFromStat);
				AddIncomingDamageModifier(DamageTakenModFromStat);
			}
			if (StatHandler->GetStatValid(HealingDoneStatTag()))
			{
				FHealingModCondition HealingDoneModFromStat;
				HealingDoneModFromStat.BindDynamic(this, &UDamageHandler::ModifyHealingDoneFromStat);
				AddOutgoingHealingModifier(HealingDoneModFromStat);
			}
			if (StatHandler->GetStatValid(HealingTakenStatTag()))
			{
				FHealingModCondition HealingTakenModFromStat;
				HealingTakenModFromStat.BindDynamic(this, &UDamageHandler::ModifyHealingTakenFromStat);
				AddIncomingHealingModifier(HealingTakenModFromStat);
			}
		}

		//Add buff restrictions for damage and healing interactions that are not enabled.
		if (IsValid(BuffHandler))
		{
			if (!CanEverDealDamage())
			{
				FBuffEventCondition DamageBuffRestriction;
				DamageBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictDamageBuffs);
				BuffHandler->AddOutgoingBuffRestriction(DamageBuffRestriction);
			}

			if (!CanEverReceiveDamage())
			{
				FBuffEventCondition DamageBuffRestriction;
				DamageBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictDamageBuffs);
				BuffHandler->AddIncomingBuffRestriction(DamageBuffRestriction);
			}

			if (!CanEverDealHealing())
			{
				FBuffEventCondition HealingBuffRestriction;
				HealingBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictHealingBuffs);
				BuffHandler->AddOutgoingBuffRestriction(HealingBuffRestriction);
			}

			if (!CanEverReceiveHealing())
			{
				FBuffEventCondition HealingBuffRestriction;
				HealingBuffRestriction.BindDynamic(this, &UDamageHandler::RestrictHealingBuffs);
				BuffHandler->AddIncomingBuffRestriction(HealingBuffRestriction);
			}
		}
		LifeStatus = ELifeStatus::Alive;
		OnLifeStatusChanged.Broadcast(GetOwner(), ELifeStatus::Invalid, LifeStatus);
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
	if (StatTag.IsValid() && StatTag.MatchesTag(MaxHealthStatTag()))
	{
		UpdateMaxHealth(NewValue);
	}
}


bool UDamageHandler::CheckDeathRestricted(FDamageInfo const& DamageInfo)
{
	for (FDamageRestriction Condition : DeathConditions)
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
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousStatus, LifeStatus);
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
	OnLifeStatusChanged.Broadcast(GetOwner(), PreviousValue, LifeStatus);
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

void UDamageHandler::AddDeathRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		DeathConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveDeathRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
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
	for (FDamageRestriction Condition : OutgoingDamageConditions)
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
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(DamageDoneStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(DamageDoneStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetOutgoingDamageMods(FDamageInfo const& DamageInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (TTuple<int32, FDamageModCondition>& Mod : OutgoingDamageModifiers)
	{
		if (Mod.Value.IsBound())
		{
			RelevantMods.Add(Mod.Value.Execute(DamageInfo));
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

void UDamageHandler::AddOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingDamageConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingDamageConditions.RemoveSingleSwap(Restriction);
	}
}

int32 UDamageHandler::AddOutgoingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FCombatModifier::GetID();
	OutgoingDamageModifiers.Add(ModID, Modifier);
	return ModID;
}

void UDamageHandler::RemoveOutgoingDamageModifier(int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || ModifierID == -1)
	{
		return;
	}
	OutgoingDamageModifiers.Remove(ModifierID);
}

void UDamageHandler::MulticastNotifyOfOutgoingDamageSuccess_Implementation(FDamagingEvent DamageEvent)
{
	OnOutgoingDamage.Broadcast(DamageEvent);
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
	for (FHealingRestriction const& Condition : OutgoingHealingConditions)
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
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(HealingDoneStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(HealingDoneStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetOutgoingHealingMods(FHealingInfo const& HealingInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (TTuple<int32, FHealingModCondition>& Mod : OutgoingHealingModifiers)
	{
		if (Mod.Value.IsBound())
		{
			RelevantMods.Add(Mod.Value.Execute(HealingInfo));
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

void UDamageHandler::AddOutgoingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingHealingConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveOutgoingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		OutgoingHealingConditions.RemoveSingleSwap(Restriction);
	}
}

int32 UDamageHandler::AddOutgoingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FCombatModifier::GetID();
	OutgoingHealingModifiers.Add(ModID, Modifier);
	return ModID;
}

void UDamageHandler::RemoveOutgoingHealingModifier(int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || ModifierID == -1)
	{
		return;
	}
	OutgoingHealingModifiers.Remove(ModifierID);
}

void UDamageHandler::MulticastNotifyOfOutgoingHealingSuccess_Implementation(FHealingEvent HealingEvent)
{
	OnOutgoingHealing.Broadcast(HealingEvent);
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
	for (FDamageRestriction const& Condition : IncomingDamageConditions)
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
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(DamageTakenStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(DamageTakenStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetIncomingDamageMods(FDamageInfo const& DamageInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (TTuple<int32, FDamageModCondition>& Mod : IncomingDamageModifiers)
	{
		if (Mod.Value.IsBound())
		{
			RelevantMods.Add(Mod.Value.Execute(DamageInfo));
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

void UDamageHandler::AddIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingDamageConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingDamageConditions.RemoveSingleSwap(Restriction);
	}
}

int32 UDamageHandler::AddIncomingDamageModifier(FDamageModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FCombatModifier::GetID();
	IncomingDamageModifiers.Add(ModID, Modifier);
	return ModID;
}

void UDamageHandler::RemoveIncomingDamageModifier(int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || ModifierID == 0)
	{
		return;
	}
	IncomingDamageModifiers.Remove(ModifierID);
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
	for (FHealingRestriction const& Condition : IncomingHealingConditions)
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
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(HealingTakenStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(HealingTakenStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

TArray<FCombatModifier> UDamageHandler::GetIncomingHealingMods(FHealingInfo const& HealingInfo)
{
	TArray<FCombatModifier> RelevantMods;
	for (TTuple<int32, FHealingModCondition>& Mod : IncomingHealingModifiers)
	{
		if (Mod.Value.IsBound())
		{
			RelevantMods.Add(Mod.Value.Execute(HealingInfo));
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

void UDamageHandler::AddIncomingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingHealingConditions.AddUnique(Restriction);
	}
}

void UDamageHandler::RemoveIncomingHealingRestriction(FHealingRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Restriction.IsBound())
	{
		IncomingHealingConditions.RemoveSingleSwap(Restriction);
	}
}

int32 UDamageHandler::AddIncomingHealingModifier(FHealingModCondition const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FCombatModifier::GetID();
	IncomingHealingModifiers.Add(ModID, Modifier);
	return ModID;
}

void UDamageHandler::RemoveIncomingHealingModifier(int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || ModifierID == -1)
	{
		return;
	}
	IncomingHealingModifiers.Remove(ModifierID);
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
*/