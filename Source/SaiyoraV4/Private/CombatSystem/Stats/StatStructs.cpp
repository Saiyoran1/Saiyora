#include "StatStructs.h"
#include "Buff.h"

void FCombatStat::SubscribeToStatChanged(const FStatCallback& Callback)
{
	if (Callback.IsBound() && Defaults.bModifiable)
	{
		OnStatChanged.AddUnique(Callback);
	}
}

void FCombatStat::UnsubscribeFromStatChanged(const FStatCallback& Callback)
{
	if (Callback.IsBound() && Defaults.bModifiable)
	{
		OnStatChanged.Remove(Callback);
	}
}

FCombatStat::FCombatStat(const FStatInfo& InitInfo)
{
	if (!InitInfo.StatTag.IsValid() || !InitInfo.StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || InitInfo.StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return;
	}
	Defaults = InitInfo;
	RecalculateValue();
	bInitialized = true;
}

void FCombatStat::AddModifier(const FCombatModifier& Modifier)
{
	if (!Defaults.bModifiable || !IsValid(Modifier.BuffSource))
	{
		return;
	}
	Modifiers.Add(Modifier.BuffSource, Modifier);
	RecalculateValue();
}

void FCombatStat::RemoveModifier(const UBuff* Source)
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
	for (const FStatCallback& Callback : Callbacks)
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