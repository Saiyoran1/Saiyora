#include "StatStructs.h"
#include "Buff.h"

void FCombatStat::PostReplicatedAdd(const FCombatStatArray& InArraySerializer)
{
	if (!bInitialized)
	{
		TArray<FStatCallback> Callbacks;
		InArraySerializer.PendingSubscriptions.MultiFind(StatTag, Callbacks);
		for (const FStatCallback& Callback : Callbacks)
		{
			OnStatChanged.AddUnique(Callback);
		}
		//TODO: Make non-const and clear out the pending subscriptions?
		bInitialized = true;
	}
	OnStatChanged.Broadcast(StatTag, StatValue.GetCurrentValue());
}

void FCombatStat::PostReplicatedChange(const FCombatStatArray& InArraySerializer)
{
	if (!bInitialized)
	{
		TArray<FStatCallback> Callbacks;
		InArraySerializer.PendingSubscriptions.MultiFind(StatTag, Callbacks);
		for (const FStatCallback& Callback : Callbacks)
		{
			OnStatChanged.AddUnique(Callback);
		}
		//TODO: Make non-const and clear out the pending subscriptions?
		bInitialized = true;
	}
	OnStatChanged.Broadcast(StatTag, StatValue.GetCurrentValue());
}