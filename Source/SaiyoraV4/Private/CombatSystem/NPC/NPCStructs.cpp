#include "NPCStructs.h"
#include "ChoiceRequirements.h"
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

bool FNPCCombatChoice::IsChoiceValid() const
{
	const UCombatAbility* AbilityInstance = OwningComponentRef->FindActiveAbility(AbilityClass);
	if (!IsValid(AbilityInstance))
	{
		return false;
	}
	TArray<ECastFailReason> FailReasons;
	bool bIsCastable = AbilityInstance->IsCastable(FailReasons);
	//We will say the choice is valid even if the ability isn't castable, if the reason it's not castable is because of movement.
	//We will stop our movement before casting the ability, so this shouldn't matter.
	//TODO: This should check whether we are moving of our own volition or because of an external move (or gravity?).
	bIsCastable = bIsCastable || FailReasons.Num() == 1 && FailReasons[0] == ECastFailReason::Moving;
	if (!bIsCastable)
	{
		return false;
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