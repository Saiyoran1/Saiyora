#include "StatStructs.h"
#include "Buff.h"
#include "StatHandler.h"

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
	if (!InitInfo.StatTag.IsValid() || !InitInfo.StatTag.MatchesTag(UStatHandler::GenericStatTag()) || InitInfo.StatTag.MatchesTagExact(UStatHandler::GenericStatTag()))
	{
		return;
	}
	Defaults = InitInfo;
	RecalculateValue();
	bInitialized = true;
}

FCombatStat::FCombatStat()
{
	//Needs to exist to prevent compile errors.
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