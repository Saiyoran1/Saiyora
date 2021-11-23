#include "CombatGroup.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

void UCombatGroup::SubscribeToCombatantAdded(FCombatantCallback const& Callback)
{
	//No good way to protect against calling this on clients, but the delegate will never trigger anyway so...
	if (Callback.IsBound())
	{
		OnCombatantAdded.AddUnique(Callback);
	}
}

void UCombatGroup::UnsubscribeFromCombatantAdded(FCombatantCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCombatantAdded.Remove(Callback);
	}
}

void UCombatGroup::SubscribeToCombatantRemoved(FCombatantCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCombatantRemoved.AddUnique(Callback);
	}
}

void UCombatGroup::UnsubscribeFromCombatantRemoved(FCombatantCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCombatantRemoved.Remove(Callback);
	}
}

void UCombatGroup::JoinCombat(AActor* NewCombatant)
{
	if (!IsValid(NewCombatant) || !NewCombatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(NewCombatant);
	if (!IsValid(CombatantThreat) || CombatantThreat->IsInCombat() || (!CombatantThreat->CanBeInThreatTable() && !CombatantThreat->HasThreatTable()))
	{
		return;
	}
	Combatants.Add(NewCombatant);
	CombatantThreat->NotifyOfCombatJoined(this);
	OnCombatantAdded.Broadcast(this, NewCombatant);
}
