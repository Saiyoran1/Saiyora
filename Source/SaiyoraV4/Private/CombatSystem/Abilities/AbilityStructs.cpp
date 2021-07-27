#include "AbilityStructs.h"
#include "CombatAbility.h"

void FReplicatedAbilityCost::PostReplicatedChange(FReplicatedAbilityCostArray const& InArraySerializer)
{
	if (IsValid(InArraySerializer.Ability))
	{
		InArraySerializer.Ability->NotifyOfReplicatedCost(AbilityCost);
	}
}

void FReplicatedAbilityCost::PostReplicatedAdd(FReplicatedAbilityCostArray const& InArraySerializer)
{
	if (IsValid(InArraySerializer.Ability))
	{
		InArraySerializer.Ability->NotifyOfReplicatedCost(AbilityCost);
	}
}

void FReplicatedAbilityCostArray::UpdateAbilityCost(FAbilityCost const& NewCost)
{
	if (IsValid(NewCost.ResourceClass))
	{
		for (FReplicatedAbilityCost& AbilityCost : Items)
		{
			if (AbilityCost.AbilityCost.ResourceClass == NewCost.ResourceClass)
			{
				if (AbilityCost.AbilityCost.Cost != NewCost.Cost || AbilityCost.AbilityCost.bStaticCost != NewCost.bStaticCost)
				{
					AbilityCost.AbilityCost = NewCost;
					MarkItemDirty(AbilityCost);
				}
				return;
			}
		}
	}
}