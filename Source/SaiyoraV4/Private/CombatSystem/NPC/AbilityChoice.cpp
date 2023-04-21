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
