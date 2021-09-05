#include "AbilityStructs.h"
#include "AbilityHandler.h"
#include "StatHandler.h"

FReplicableAbilityCost::FReplicableAbilityCost()
{
	Cost = 0.0f;
	ResourceClass = nullptr;
}

FReplicableAbilityCost::FReplicableAbilityCost(TSubclassOf<UResource> const NewResourceClass, float const NewCost)
{
	ResourceClass = NewResourceClass;
	Cost = NewCost;
}