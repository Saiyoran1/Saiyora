#include "AbilityStructs.h"
#include "CombatAbility.h"

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

FAbilityTargetSet::FAbilityTargetSet()
{
	//Needs to exist to prevent compile errors;
}

FAbilityTargetSet::FAbilityTargetSet(int32 const SetID, TArray<AActor*> const& Targets)
{
	this->SetID = SetID;
	this->Targets = Targets;
}

FAbilityParams::FAbilityParams()
{
	//Needs to exist to prevent compile errors;
}

FAbilityParams::FAbilityParams(FAbilityOrigin const& InOrigin, TArray<FAbilityTargetSet> const& InTargets)
{
	this->Origin = InOrigin;
	this->Targets = InTargets;
}

FPredictedTick::FPredictedTick()
{
	//Needs to exist to prevent compile errors.
	this->PredictionID = 0;
	this->TickNumber = 0;
}
