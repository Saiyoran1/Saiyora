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

void FNPCCombatChoice::DEBUG_GetDisplayInfo(TArray<FString>& OutInfo) const
{
	OutInfo.Empty();
	for (const FInstancedStruct& InstancedReq : Requirements)
	{
		const FNPCChoiceRequirement* Requirement = InstancedReq.GetPtr<FNPCChoiceRequirement>();
		if (Requirement)
		{
			OutInfo.Add(Requirement->DEBUG_GetReqName() + (Requirement->IsMet() ? "Met" : "Failed"));
		}
	}
}

void FNPCChoiceRequirement::Init(FNPCCombatChoice* Choice, AActor* NPC)
{
	OwningChoice = Choice;
	OwningNPC = NPC;
	OwningChoice = Choice;
	SetupRequirement();
}