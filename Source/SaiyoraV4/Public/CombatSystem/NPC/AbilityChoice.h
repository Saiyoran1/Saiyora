#pragma once
#include "CoreMinimal.h"
#include "CombatStructs.h"
#include "Object.h"
#include "AbilityChoice.generated.h"

class UCombatAbility;
class UNPCBehavior;

UCLASS(Blueprintable)
class SAIYORAV4_API UAbilityChoice : public UObject
{
	GENERATED_BODY()

public:
	
	void InitializeChoice(UNPCBehavior* BehaviorComp);
	
	UFUNCTION(BlueprintImplementableEvent)
	void Setup();
	UFUNCTION(BlueprintImplementableEvent)
	void CleanUp();

	UFUNCTION(BlueprintCallable)
	FCombatModifierHandle ApplyScoreModifier(const FCombatModifier& Modifier) { return CurrentScore.AddModifier(Modifier); }
	UFUNCTION(BlueprintCallable)
	void RemoveScoreModifier(const FCombatModifierHandle& ModifierHandle) { CurrentScore.RemoveModifier(ModifierHandle); }
	UFUNCTION(BlueprintCallable)
	void UpdateScoreModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier) { CurrentScore.UpdateModifier(ModifierHandle, Modifier); }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float DefaultScore = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bHighPriority = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bRequiresRange = true;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float MaxRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float OutOfRangeScoreMultiplier = 0.0f;
	FCombatModifierHandle RangeModifierHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	bool bRequiresLineOfSight = true;
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	float OutOfLineMultiplier = 0.0f;
	FCombatModifierHandle LineModifierHandle;

	UPROPERTY()
	UNPCBehavior* BehaviorComponentRef = nullptr;
	FModifiableFloat CurrentScore;
};
