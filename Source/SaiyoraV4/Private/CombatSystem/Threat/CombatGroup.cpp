#include "CombatGroup.h"
#include "CombatStatusComponent.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

void UCombatGroup::AddCombatant(UThreatHandler* Combatant)
{
	const UCombatStatusComponent* CombatantCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant->GetOwner());
	if (!IsValid(CombatantCombat))
	{
		return;
	}
	const bool bIsFriendly = CombatantCombat->GetCurrentFaction() == EFaction::Friendly;
	if (bIsFriendly)
	{
		if (Friendlies.Contains(Combatant))
		{
			return;
		}
		Friendlies.Add(Combatant);
	}
	else
	{
		if (Enemies.Contains(Combatant))
		{
			return;
		}
		Enemies.Add(Combatant);
	}
	UDamageHandler* CombatantDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Combatant->GetOwner());
	if (IsValid(CombatantDamage))
	{
		CombatantDamage->OnIncomingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantHealthEvent);
		CombatantDamage->OnLifeStatusChanged.AddDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
	}
	Combatant->NotifyOfCombat(this);
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		Enemy->NotifyOfNewCombatant(Combatant);
		Combatant->NotifyOfNewCombatant(Enemy);
	}
}

void UCombatGroup::RemoveCombatant(UThreatHandler* Combatant)
{
	if (!IsValid(Combatant))
	{
		return;
	}
	const UCombatStatusComponent* CombatantCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant->GetOwner());
	if (!IsValid(CombatantCombat))
	{
		return;
	}
	const bool bIsFriendly = CombatantCombat->GetCurrentFaction() == EFaction::Friendly;
	if (bIsFriendly)
	{
		if (Friendlies.Remove(Combatant) <= 0)
		{
			return;
		}
	}
	else
	{
		if (Enemies.Remove(Combatant) <= 0)
		{
			return;
		}
	}
	UDamageHandler* CombatantDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Combatant->GetOwner());
	if (IsValid(CombatantDamage))
	{
		CombatantDamage->OnIncomingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantHealthEvent);
		CombatantDamage->OnLifeStatusChanged.RemoveDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
		//TODO: Unbind from outgoing healing?
	}
	Combatant->NotifyOfCombat(nullptr);
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		Enemy->NotifyOfCombatantLeft(Combatant);
	}
}

void UCombatGroup::OnCombatantHealthEvent(const FHealthEvent& Event)
{
	if (!Event.Result.Success)
	{
		return;
	}
	//TODO: On damage taken, add new combatant.
	//Damage from A to B -> B's ThreatHandler notified
	//Threat Handler calls:
	//1a: Not in combat, enemy also not in combat:
	// CreateCombatGroup(A, B) -> AddCombatant(A) -> NotifyOfCombat(A) -> AddCombatant(B) -> NotifyOfCombat(B)
	//1b: Not in combat, enemy is in combat:
	// AddCombatant(B) -> NotifyOfCombat(B)
	//2a: In combat, enemy not in combat:
	// AddCombatant(A) -> NotifyOfCombat(A)
	//2b: In combat, enemy in combat:
	// MergeCombatGroup(A->GetCombatGroup())
	//TODO: On healing/absorb taken, generate threat for enemy combatants, add new combatant if needed.
}

void UCombatGroup::OnCombatantLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive && PreviousStatus == ELifeStatus::Alive &&
		IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UThreatHandler* Combatant = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
		if (IsValid(Combatant))
		{
			RemoveCombatant(Combatant);
		}
	}
}

