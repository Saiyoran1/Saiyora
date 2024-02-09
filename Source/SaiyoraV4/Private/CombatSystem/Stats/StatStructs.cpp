#include "StatStructs.h"
#include "Buff.h"

void FCombatStat::PostReplicatedAdd(const FCombatStatArray& InArraySerializer)
{
	//If we haven't initialized this specific stat on this client, we need to handle any pending subscriptions.
	if (!bInitialized)
	{
		//These are delegates that were waiting on replication to bind to value changes in this stat.
		TArray<FStatCallback> Callbacks;
		InArraySerializer.PendingSubscriptions.MultiFind(StatTag, Callbacks);
		for (const FStatCallback& Callback : Callbacks)
		{
			OnStatChanged.AddUnique(Callback);
		}
		//TODO: Make non-const and clear out the pending subscriptions?
		bInitialized = true;
	}
	OnStatChanged.Broadcast(StatTag, GetCurrentValue());
}

void FCombatStat::PostReplicatedChange(const FCombatStatArray& InArraySerializer)
{
	//If we haven't initialized this specific stat on this client, we need to handle any pending subscriptions.
	if (!bInitialized)
	{
		//These are delegates that were waiting on replication to bind to value changes in this stat.
		TArray<FStatCallback> Callbacks;
		InArraySerializer.PendingSubscriptions.MultiFind(StatTag, Callbacks);
		for (const FStatCallback& Callback : Callbacks)
		{
			OnStatChanged.AddUnique(Callback);
		}
		//TODO: Make non-const and clear out the pending subscriptions?
		bInitialized = true;
	}
	OnStatChanged.Broadcast(StatTag, GetCurrentValue());
}