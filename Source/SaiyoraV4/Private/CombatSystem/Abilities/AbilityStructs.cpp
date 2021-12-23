#include "AbilityStructs.h"
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