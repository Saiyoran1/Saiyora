#include "AbilityChoice.h"
#include "NPCBehavior.h"

void UAbilityChoice::InitializeChoice(UNPCBehavior* BehaviorComp)
{
	if (!IsValid(BehaviorComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid behavior component reference passed to ability choice."));
		return;
	}
	BehaviorComponentRef = BehaviorComp;
	CurrentScore = FModifiableFloat(DefaultScore, true, true, 0.0f);
	Setup();
	const FModifiableFloatCallback ScoreCallback = FModifiableFloatCallback::CreateUObject(this, &UAbilityChoice::ScoreCallback);
	CurrentScore.SetUpdatedCallback(ScoreCallback);
	OnScoreChanged.ExecuteIfBound(this, CurrentScore.GetCurrentValue());
}

void UAbilityChoice::UpdateRangeAndLos(const float Range, const bool LineOfSight)
{
	if (!bUsesDefaultTargeting)
	{
		//Abilities that target things other than the highest threat target need to manually check LoS and range if they want multipliers.
		return;
	}
	if (OutOfRangeModifier != 1.0f)
	{
		if (Range > AbilityRange && !RangeModifierHandle.IsValid())
		{
			RangeModifierHandle = ApplyScoreMultiplier(OutOfRangeModifier);
		}
		else if (Range <= AbilityRange && RangeModifierHandle.IsValid())
		{
			RemoveScoreMultiplier(RangeModifierHandle);
			RangeModifierHandle = FCombatModifierHandle::Invalid;
		}
	}
	if (OutOfLosModifier != 1.0f)
	{
		if (LineOfSight && LosModifierHandle.IsValid())
		{
			RemoveScoreMultiplier(LosModifierHandle);
			LosModifierHandle = FCombatModifierHandle::Invalid;
		}
		else if (!LineOfSight && !LosModifierHandle.IsValid())
		{
			LosModifierHandle = ApplyScoreMultiplier(OutOfLosModifier);
		}
	}
}

FCombatModifierHandle UAbilityChoice::ApplyScoreMultiplier(const float Multiplier)
{
	if (Multiplier < 0.0f)
	{
		return FCombatModifierHandle::Invalid;
	}
	return CurrentScore.AddModifier(FCombatModifier(Multiplier, EModifierType::Multiplicative));
}

void UAbilityChoice::UpdateScoreMultiplier(const FCombatModifierHandle& ModifierHandle, const float Multiplier)
{
	if (Multiplier < 0.0f)
	{
		CurrentScore.RemoveModifier(ModifierHandle);
	}
	else
	{
		CurrentScore.UpdateModifier(ModifierHandle, FCombatModifier(Multiplier, EModifierType::Multiplicative));
	}
}
