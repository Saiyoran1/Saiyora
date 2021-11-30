#include "CombatStat.h"

#include "Buff.h"
#include "StatHandler.h"

/*void UCombatStat::Init(FStatInfo const& InitInfo, UStatHandler* NewHandler)
{
	if (!IsValid(NewHandler) || !InitInfo.StatTag.MatchesTag(UStatHandler::GenericStatTag()))
	{
		return;
	}
	Handler = NewHandler;
	StatTag = InitInfo.StatTag;
	bShouldReplicate = InitInfo.bShouldReplicate;
	UModifiableFloatValue::Init(InitInfo.DefaultValue, InitInfo.bModifiable, InitInfo.bCappedLow, InitInfo.MinClamp, InitInfo.bCappedHigh, InitInfo.MaxClamp);
	bInitialized = true;
}

void UCombatStat::OnValueChanged(float const PreviousValue, int32 const PredictionID)
{
	OnStatChanged.Broadcast(StatTag, GetValue());
}

void UCombatStat::SubscribeToStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnStatChanged.AddUnique(Callback);
	}
}

void UCombatStat::UnsubscribeFromStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnStatChanged.Remove(Callback);
	}
}*/


void FCombatStat::SubscribeToStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnStatChanged.AddUnique(Callback);
	}
}

void FCombatStat::UnsubscribeFromStatChanged(FStatCallback const& Callback)
{
	if (Callback.IsBound())
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

void FCombatStat::AddModifier(FCombatModifier const& Modifier)
{
	if (!Defaults.bModifiable || !IsValid(Modifier.Source))
	{
		return;
	}
	bool bFound = false;
	bool bChanged = false;
	int32 const PreviousID = LastPredictionID;
	for (FCombatModifier& Mod : Modifiers)
	{
		if (IsValid(Mod.Source) && Mod.Source == Modifier.Source)
		{
			if (Mod.bStackable)
			{
				Mod.Stacks = Modifier.Stacks;
				bChanged = true;
			}
			LastPredictionID = FMath::Max(LastPredictionID, Modifier.Source->GetLastPredictionID());
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		Modifiers.Add(Modifier);
		bChanged = true;
		LastPredictionID = FMath::Max(LastPredictionID, Modifier.Source->GetLastPredictionID());
	}
	if (LastPredictionID != PreviousID)
	{
		bChanged = true;
	}
	if (bChanged)
	{
		RecalculateValue();
	}
}

void FCombatStat::RemoveModifier(UBuff* Source)
{
	if (!IsValid(Source))
	{
		return;
	}
	int32 RemoveIndex = -1;
	for (int i = 0; i < Modifiers.Num(); i++)
	{
		if (IsValid(Modifiers[i].Source) && Modifiers[i].Source == Source)
		{
			RemoveIndex = i;
			break;
		}
	}
	if (RemoveIndex != -1)
	{
		Modifiers.RemoveAt(RemoveIndex);
		RecalculateValue();
	}
}

void FCombatStat::PostReplicatedAdd(const FCombatStatArray& InArraySerializer)
{
	TArray<FStatCallback> Callbacks;
	InArraySerializer.PendingSubscriptions.MultiFind(Defaults.StatTag, Callbacks);
	for (FStatCallback const& Callback : Callbacks)
	{
		SubscribeToStatChanged(Callback);
	}
	RecalculateValue();
}

void FCombatStat::PostReplicatedChange(const FCombatStatArray& InArraySerializer)
{
	RecalculateValue();
}

void FCombatStat::RecalculateValue()
{
	if (Defaults.bModifiable)
	{
		Value = FCombatModifier::ApplyModifiers(Modifiers, Defaults.DefaultValue);
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
	OnStatChanged.Broadcast(Defaults.StatTag, Value, LastPredictionID);
}
