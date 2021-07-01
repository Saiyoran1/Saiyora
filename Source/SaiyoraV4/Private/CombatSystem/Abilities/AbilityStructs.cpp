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

FGlobalCooldown FPredictedGlobalCooldown::ToGlobalCooldown() const
{
	FGlobalCooldown Global;
	Global.bGlobalCooldownActive = bGlobalCooldownActive;
	Global.StartTime = StartTime;
	Global.EndTime = EndTime;
	return Global;
}

FCastingState FPredictedCastingState::ToCastingState() const
{
	FCastingState Cast;
	Cast.bIsCasting = bIsCasting;
	Cast.CurrentCast = CurrentCast;
	Cast.CastStartTime = CastStartTime;
	Cast.CastEndTime = CastEndTime;
	Cast.bInterruptible = bInterruptible;
	Cast.ElapsedTicks = ElapsedTicks;
	return Cast;
}
