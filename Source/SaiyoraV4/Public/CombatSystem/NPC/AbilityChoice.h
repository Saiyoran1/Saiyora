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
	bool RequiresTarget() const { return bRequiresTarget; }
	AActor* GetTarget() const { return Target; }
	bool RequiresLineOfSight() const { return bRequiresLineOfSight; }
	float GetAbilityRange() const { return AbilityRange; }
	bool CanUseWhileMoving() const { return bUsableWhileMoving; }
	ESaiyoraPlane GetLineOfSightPlane() const { return LineOfSightPlane; }

protected:

	UFUNCTION(BlueprintCallable)
	FCombatModifierHandle ApplyScoreMultiplier(const float Multiplier);
	UFUNCTION(BlueprintCallable)
	void RemoveScoreMultiplier(const FCombatModifierHandle& ModifierHandle) { CurrentScore.RemoveModifier(ModifierHandle); }
	UFUNCTION(BlueprintCallable)
	void UpdateScoreMultiplier(const FCombatModifierHandle& ModifierHandle, const float Multiplier);

	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* NewTarget) { Target = NewTarget; }
	UFUNCTION(BlueprintCallable)
	void SetLineOfSightPlane(const ESaiyoraPlane NewPlane) { LineOfSightPlane = NewPlane; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float DefaultScore = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bHighPriority = false;

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bRequiresTarget = false;
	UPROPERTY()
	AActor* Target = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bRequiresLineOfSight = false;
	ESaiyoraPlane LineOfSightPlane = ESaiyoraPlane::None;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float AbilityRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bUsableWhileMoving = false;
	
	UPROPERTY()
	UNPCBehavior* BehaviorComponentRef = nullptr;
	FModifiableFloat CurrentScore;
	void ScoreCallback(const float OldValue, const float NewValue) { OnScoreChanged.ExecuteIfBound(this, NewValue); }
};
