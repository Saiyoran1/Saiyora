#include "CombatGroup.h"
#include "SaiyoraGameState.h"
#include "MegaComponent/CombatComponent.h"

void UCombatGroup::Initialize(UCombatComponent* Instigator, UCombatComponent* Target)
{
	if (bInitialized || !IsValid(Instigator) || !IsValid(Target))
	{
		MarkPendingKill();
		return;
	}
	//TODO: Check actors are different factions.
	bInitialized = true;
	GameStateRef = Instigator->GetGameStateRef();
	if (!IsValid(GameStateRef))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in Combat Group!"));
		MarkPendingKill();
		return;
	}
	CombatStartTime = GameStateRef->GetServerWorldTimeSeconds();
	AddNewCombatant(Instigator);
	AddNewCombatant(Target);
}

void UCombatGroup::AddNewCombatant(UCombatComponent* Combatant)
{
	if (!IsValid(Combatant) || CombatActors.Contains(Combatant))
	{
		return;
	}
	
}