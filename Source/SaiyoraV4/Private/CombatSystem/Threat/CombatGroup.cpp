#include "CombatGroup.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

void UCombatGroup::JoinCombat(AActor* NewCombatant)
{
	checkf(IsValid(NewCombatant), TEXT("Invalid actor added to a combat group."));
	
	if (!NewCombatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(NewCombatant);
	if (!IsValid(CombatantThreat) || CombatantThreat->IsInCombat() || (!CombatantThreat->CanBeInThreatTable() && !CombatantThreat->HasThreatTable()))
	{
		return;
	}
	Combatants.Add(NewCombatant);
	for (AActor* Combatant : Combatants)
	{
		if (IsValid(Combatant) && Combatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* NotifyThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Combatant);
			if (IsValid(NotifyThreat))
			{
				NotifyThreat->NotifyOfNewCombatant(NewCombatant);
			}
		}
	}
	CombatantThreat->NotifyOfCombatJoined(this);
}

void UCombatGroup::LeaveCombat(AActor* LeavingCombatant)
{
	checkf(IsValid(LeavingCombatant), TEXT("Attempted to remove an invalid combatant from combat."));
	
	int32 const Removed = Combatants.Remove(LeavingCombatant);
	if (Removed > 0)
	{
		for (AActor* Combatant : Combatants)
		{
			if (IsValid(Combatant) && Combatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
			{
				UThreatHandler* NotifyThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Combatant);
				if (IsValid(NotifyThreat))
				{
					NotifyThreat->NotifyOfRemovedCombatant(LeavingCombatant);
				}
			}
		}
		if (LeavingCombatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(LeavingCombatant);
			if (IsValid(CombatantThreat))
			{
				CombatantThreat->NotifyOfCombatLeft(this);
			}
		}
	}
}

void UCombatGroup::MergeGroups(UCombatGroup* OtherGroup)
{
	checkf(IsValid(OtherGroup), TEXT("Attempted to merge into an invalid combat group."));

	if (bInMerge)
	{
		return;
	}
	bInMerge = true;
	OtherGroup->NotifyOfMergeStart();
	TArray<AActor*> OtherCombatants;
	OtherGroup->GetActorsInCombat(OtherCombatants);
	for (AActor* Combatant : OtherCombatants)
	{
		if (IsValid(Combatant) && Combatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Combatant);
			if (IsValid(CombatantThreat))
			{
				CombatantThreat->NotifyOfCombatGroupMerge(OtherGroup, this);
			}
		}
	}
	for (AActor* Combatant : Combatants)
	{
		if (IsValid(Combatant) && Combatant->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Combatant);
			if (IsValid(CombatantThreat))
			{
				CombatantThreat->NotifyOfCombatGroupMerge(OtherGroup, this);
			}
		}
	}
	Combatants.Append(OtherCombatants);
	OtherGroup->NotifyOfMergeComplete();
	bInMerge = false;
}