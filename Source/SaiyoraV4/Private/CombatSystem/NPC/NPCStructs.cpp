#include "NPCStructs.h"
#include "NPCAbility.h"
#include "NPCAbilityComponent.h"

void FNPCCombatChoice::Init(UNPCAbilityComponent* AbilityComponent)
{
	if (bInitialized)
	{
		return;
	}
	OwningComponentRef = AbilityComponent;
	for (int i = Requirements.Num() - 1; i >= 0; i--)
	{
		FNPCChoiceRequirement* CastRequirement = Requirements[i].GetMutablePtr<FNPCChoiceRequirement>();
		if (!CastRequirement)
		{
			Requirements.RemoveAt(i);
			continue;
		}
		CastRequirement->Init(this, AbilityComponent->GetOwner());
	}
	bInitialized = true;
}

bool FNPCCombatChoice::HasAbility() const
{
	return IsValid(AbilityClass);
}

bool FNPCCombatChoice::IsChoiceValid() const
{
	if (HasAbility())
	{
		UCombatAbility* AbilityInstance = OwningComponentRef->FindActiveAbility(AbilityClass);
		TArray<ECastFailReason> FailReasons;
		if (!IsValid(AbilityInstance) || !AbilityInstance->IsCastable(FailReasons))
		{
			return false;
		}
	}
	for (const FInstancedStruct& InstancedRequirement : Requirements)
	{
		const FNPCChoiceRequirement* Requirement = InstancedRequirement.GetPtr<FNPCChoiceRequirement>();
		if (Requirement && !Requirement->IsMet())
		{
			return false;		
		}
	}
	return true;
}

void FNPCChoiceRequirement::Init(FNPCCombatChoice* Choice, AActor* NPC)
{
	OwningChoice = Choice;
	OwningNPC = NPC;
	OwningChoice = Choice;
	SetupRequirement();
}