#include "AbilityStructs.h"
#include "CombatAbility.h"
#include "StatHandler.h"

FAbilityCost::FAbilityCost()
{
	Cost = 0.0f;
	ResourceClass = nullptr;
}

FAbilityCost::FAbilityCost(TSubclassOf<UResource> const NewResourceClass, float const NewCost)
{
	ResourceClass = NewResourceClass;
	Cost = NewCost;
}

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

FPredictedTick::FPredictedTick()
{
	//Needs to exist to prevent compile errors.
}