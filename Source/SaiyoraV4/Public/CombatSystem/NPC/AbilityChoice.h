#pragma once
#include "CoreMinimal.h"
#include "CombatStructs.h"
#include "Object.h"
#include "AbilityChoice.generated.h"

class UCombatAbility;
class UNPCBehavior;
class UAbilityChoice;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FChoiceScoreCallback, UAbilityChoice*, Choice, const float, NewScore);

UCLASS(Blueprintable)
class SAIYORAV4_API UAbilityChoice : public UObject
{
	GENERATED_BODY()

public:
	
	void InitializeChoice(UNPCBehavior* BehaviorComp);
	FChoiceScoreCallback OnScoreChanged;
	
	UFUNCTION(BlueprintImplementableEvent)
	void Setup();
	UFUNCTION(BlueprintImplementableEvent)
	void CleanUp();

	float GetCurrentScore() const { return CurrentScore.GetCurrentValue(); }
	bool IsHighPriority() const { return bHighPriority; }
	TSubclassOf<UCombatAbility> GetAbilityClass() const { return AbilityClass; }
	bool CanUseWhileMoving() const { return bUsableWhileMoving; }

	void UpdateRangeAndLos(const float Range, const bool LineOfSight);

protected:

	UFUNCTION(BlueprintCallable)
	FCombatModifierHandle ApplyScoreMultiplier(const float Multiplier);
	UFUNCTION(BlueprintCallable)
	void RemoveScoreMultiplier(const FCombatModifierHandle& ModifierHandle) { CurrentScore.RemoveModifier(ModifierHandle); }
	UFUNCTION(BlueprintCallable)
	void UpdateScoreMultiplier(const FCombatModifierHandle& ModifierHandle, const float Multiplier);

	UFUNCTION(BlueprintCallable)
	void SetAlternateTarget(AActor* Target) { AlternateTarget = Target; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float DefaultScore = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bHighPriority = false;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bUsesDefaultTargeting = true;
	UPROPERTY()
	AActor* AlternateTarget = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float OutOfLosModifier = 1.0f;
	FCombatModifierHandle LosModifierHandle;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float OutOfRangeModifier = 1.0f;
	FCombatModifierHandle RangeModifierHandle;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float AbilityRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bUsableWhileMoving = false;
	
	UPROPERTY()
	UNPCBehavior* BehaviorComponentRef = nullptr;
	FModifiableFloat CurrentScore;
	void ScoreCallback(const float OldValue, const float NewValue) { OnScoreChanged.ExecuteIfBound(this, NewValue); }
};
