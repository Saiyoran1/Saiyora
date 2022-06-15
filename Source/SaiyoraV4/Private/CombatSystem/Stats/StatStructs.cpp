#include "StatStructs.h"
#include "Buff.h"

const FGameplayTag FStatTags::GenericStat = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false);
const FGameplayTag FStatTags::DamageDone = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageDone")), false);
const FGameplayTag FStatTags::DamageTaken = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageTaken")), false);
const FGameplayTag FStatTags::HealingDone = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingDone")), false);
const FGameplayTag FStatTags::HealingTaken = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingTaken")), false);
const FGameplayTag FStatTags::MaxHealth = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxHealth")), false);
const FGameplayTag FStatTags::GlobalCooldownLength = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false);
const FGameplayTag FStatTags::CastLength = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false);
const FGameplayTag FStatTags::CooldownLength = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false);
const FGameplayTag FStatTags::MaxWalkSpeed = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxWalkSpeed")), false);
const FGameplayTag FStatTags::MaxCrouchSpeed = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxCrouchSpeed")), false);
const FGameplayTag FStatTags::GroundFriction = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GroundFriction")), false);
const FGameplayTag FStatTags::BrakingDeceleration = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.BrakingDeceleration")), false);
const FGameplayTag FStatTags::MaxAcceleration = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxAcceleration")), false);
const FGameplayTag FStatTags::GravityScale = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GravityScale")), false);
const FGameplayTag FStatTags::JumpZVelocity = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.JumpZVelocity")), false);
const FGameplayTag FStatTags::AirControl = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.AirControl")), false);

void FCombatStat::SubscribeToStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound() && Defaults.bModifiable)
	{
		OnStatChanged.AddUnique(Callback);
	}
}

void FCombatStat::UnsubscribeFromStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound() && Defaults.bModifiable)
	{
		OnStatChanged.Remove(Callback);
	}
}

FCombatStat::FCombatStat(FStatInfo const& InitInfo)
{
	if (!InitInfo.StatTag.IsValid() || !InitInfo.StatTag.MatchesTag(FStatTags::GenericStat) || InitInfo.StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return;
	}
	Defaults = InitInfo;
	RecalculateValue();
	bInitialized = true;
}

void FCombatStat::AddModifier(FCombatModifier const& Modifier)
{
	if (!Defaults.bModifiable || !IsValid(Modifier.Source))
	{
		return;
	}
	Modifiers.Add(Modifier.Source, Modifier);
	RecalculateValue();
}

void FCombatStat::RemoveModifier(UBuff* Source)
{
	if (!IsValid(Source))
	{
		return;
	}
	Modifiers.Remove(Source);
	RecalculateValue();
}

void FCombatStat::PostReplicatedAdd(const FCombatStatArray& InArraySerializer)
{
	TArray<FStatCallback> Callbacks;
	InArraySerializer.PendingSubscriptions.MultiFind(Defaults.StatTag, Callbacks);
	for (FStatCallback const& Callback : Callbacks)
	{
		SubscribeToStatChanged(Callback);
	}
	bInitialized = true;
	OnStatChanged.Broadcast(Defaults.StatTag, Value);
}

void FCombatStat::PostReplicatedChange(const FCombatStatArray& InArraySerializer)
{
	OnStatChanged.Broadcast(Defaults.StatTag, Value);
}

void FCombatStat::RecalculateValue()
{
	if (Defaults.bModifiable)
	{
		TArray<FCombatModifier> Mods;
		Modifiers.GenerateValueArray(Mods);
		Value = FCombatModifier::ApplyModifiers(Mods, Defaults.DefaultValue);
	}
	else
	{
		Value = Defaults.DefaultValue;
	}
	if (Defaults.bCappedLow)
	{
		Value = FMath::Max(Defaults.MinClamp, Value);
	}
	if (Defaults.bCappedHigh)
	{
		Value = FMath::Min(Defaults.MaxClamp, Value);
	}
	OnStatChanged.Broadcast(Defaults.StatTag, Value);
}