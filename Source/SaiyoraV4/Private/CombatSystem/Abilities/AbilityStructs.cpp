#include "AbilityStructs.h"
#include "CombatAbility.h"

void FAbilityCost::PostReplicatedAdd(const FAbilityCostArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningAbility))
	{
		InArraySerializer.OwningAbility->UpdateCostFromReplication(*this, true);
	}
}

void FAbilityCost::PostReplicatedChange(const FAbilityCostArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningAbility))
	{
		InArraySerializer.OwningAbility->UpdateCostFromReplication(*this);
	}
}
