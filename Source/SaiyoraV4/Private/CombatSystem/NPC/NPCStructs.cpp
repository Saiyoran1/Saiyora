#include "NPCStructs.h"

void FNPCAbilityChoice::SetupConditions(AActor* Owner)
{
	for (FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		FNPCAbilityCondition* Condition = InstancedCondition.GetMutablePtr<FNPCAbilityCondition>();
		if (Condition)
		{
			Condition->SetupConditionChecks(Owner, AbilityClass);
			if (bHighPriority)
			{
				Condition->OnConditionUpdated.BindRaw(this, &FNPCAbilityChoice::UpdateOnConditionMet);
			}
		}
	}
}

bool FNPCAbilityChoice::AreConditionsMet() const
{
	for (const FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		const FNPCAbilityCondition* Condition = InstancedCondition.GetPtr<FNPCAbilityCondition>();
		if (Condition && !Condition->IsConditionMet())
		{
			return false;
		}
	}
	return true;
}

void FNPCAbilityChoice::TickConditions(AActor* Owner, const float DeltaTime)
{
	for (FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		FNPCAbilityCondition* Condition = InstancedCondition.GetMutablePtr<FNPCAbilityCondition>();
		if (Condition && Condition->bRequiresTick)
		{
			Condition->TickCondition(Owner, AbilityClass, DeltaTime);
		}
	}
}

void FNPCAbilityChoice::UpdateOnConditionMet()
{
	if (AreConditionsMet())
	{
		OnChoiceAvailable.Execute(ChoiceIndex);
	}
}