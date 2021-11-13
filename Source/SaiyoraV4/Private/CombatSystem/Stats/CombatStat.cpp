#include "CombatStat.h"
#include "StatHandler.h"

void UCombatStat::BroadcastStatChanged(float const Previous, float const New)
{
	OnStatChanged.Broadcast(StatTag, New);
}

void UCombatStat::Init(FStatInfo const& InitInfo, UStatHandler* NewHandler)
{
	if (!IsValid(NewHandler) || !InitInfo.StatTag.MatchesTag(UStatHandler::GenericStatTag()))
	{
		return;
	}
	Handler = NewHandler;
	StatTag = InitInfo.StatTag;
	bShouldReplicate = InitInfo.bShouldReplicate;
	UModifiableFloatValue::Init(InitInfo.DefaultValue, InitInfo.bModifiable, InitInfo.bCappedLow, InitInfo.MinClamp, InitInfo.bCappedHigh, InitInfo.MaxClamp);
	BroadcastCallback.BindDynamic(this, &UCombatStat::BroadcastStatChanged);
	SubscribeToValueChanged(BroadcastCallback);
	bInitialized = true;
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
}
