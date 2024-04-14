#include "NPCStructs.h"
#include "NPCAbilityComponent.h"

void FNPCCombatChoice::Init(UNPCAbilityComponent* AbilityComponent, const int Index)
{
	if (bInitialized)
	{
		return;
	}
	OwningComponentRef = AbilityComponent;
	Priority = Index;
	for (int i = Requirements.Num() - 1; i >= 0; i--)
	{
		FNPCChoiceRequirement* CastRequirement = Requirements[i].GetMutablePtr<FNPCChoiceRequirement>();
		if (!CastRequirement)
		{
			Requirements.RemoveAt(i);
			continue;
		}
		CastRequirement->Init(this, AbilityComponent->GetOwner(), i);
	}
	bInitialized = true;
}

void FNPCCombatChoice::UpdateRequirementMet(const int RequirementIdx, const bool bRequirementMet)
{
	RequirementsMap.Add(RequirementIdx, bRequirementMet);
	if (!bInitialized)
	{
		return;
	}
	const bool bPreviouslyValid = bValid;
	for (const TTuple<int, bool>& Requirement : RequirementsMap)
	{
		if (!Requirement.Value)
		{
			bValid = false;
			return;
		}
	}
	bValid = true;
	if (!bPreviouslyValid)
	{
		OwningComponentRef->OnChoiceBecameValid(Priority);
	}
}

void FNPCCombatChoice::DEBUG_GetDisplayInfo(TArray<FString>& OutInfo) const
{
	OutInfo.Empty();
	OutInfo.Add(FString::Printf(L"%i: %s", Priority, *DEBUG_ChoiceName));
	OutInfo.Add(bHighPriority ? "HighPrio" : "NormalPrio");
	for (const FInstancedStruct& InstancedReq : Requirements)
	{
		const FNPCChoiceRequirement* Requirement = InstancedReq.GetPtr<FNPCChoiceRequirement>();
		if (Requirement)
		{
			OutInfo.Add(Requirement->DEBUG_GetReqName() + (Requirement->IsMet() ? "Met" : "Failed"));
		}
	}
}

void FNPCChoiceRequirement::Init(FNPCCombatChoice* Choice, AActor* NPC, const int Idx)
{
	OwningChoice = Choice;
	OwningNPC = NPC;
	RequirementIndex = Idx;
	OwningChoice = Choice;
	SetupRequirement();
}

void FNPCChoiceRequirement::UpdateRequirementMet(const bool bMet)
{
	bIsMet = bMet;
	OwningChoice->UpdateRequirementMet(RequirementIndex, bIsMet);
}