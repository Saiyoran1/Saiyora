#pragma once
#include "CoreMinimal.h"
#include "AbilityCondition.generated.h"

class UCombatAbility;

//These structs are for use as FInstancedStructs when setting up conditions for NPCs to decide whether they can use an ability or not.
//For a given condition, inherit from FNPCAbilityCondition and override CheckCondition to return whether the condition is true or not.
//Can add any variables you want to be configurable per-instance in the editor that will be used in the check.

USTRUCT()
struct FNPCAbilityCondition
{
	GENERATED_BODY()

	virtual bool CheckCondition(AActor* NPC, TSubclassOf<UCombatAbility> AbilityClass) const { return false; }

	virtual ~FNPCAbilityCondition() {}
};

USTRUCT()
struct FAbilityRangeCondition : public FNPCAbilityCondition
{
	GENERATED_BODY()

	//Whether to use the ability's range to determine whether we are close enough, or to use an override value.
	UPROPERTY(EditAnywhere)
	bool bUseAbilityRange = true;
	//If not using the ability's range, check against this range value instead.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bUseAbilityRange"))
	float RangeOverride = 100.0f;
	//If not using the ability's range, we can optionally check if we are outside the range, rather than inside.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bUseAbilityRange"))
	bool bCheckOutOfRange = false;

	virtual bool CheckCondition(AActor* NPC, TSubclassOf<UCombatAbility> AbilityClass) const override;
};