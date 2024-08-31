#pragma once
#include "CoreMinimal.h"
#include "CombatStructs.h"
#include "ResourceStructs.generated.h"

class UResource;

//Enum for how a resource's current value changes when the max value changes.
UENUM()
enum class EResourceAdjustmentBehavior : uint8
{
	//The current value remains the same when the max value changes.
	//Ex. 1: Previous: 30/100. New: 30/150.
	//Ex. 2: Previous 30/100. New: 30/50.
	None,
	//The current value is adjusted to maintain the same percentage of the max resource.
	//Ex. 1: Previous: 30/100. New: 45/150.
	//Ex. 2: Previous 30/100. New: 15/50.
	PercentOfMax,
	//The current value is adjusted to maintain the same amount of missing resource.
	//Ex. 1: Previous: 30/100. New: 80/150.
	//Ex. 2: Previous 30/100. New: 0/50. (was -20, clamped to 0)
	OffsetFromMax
};

USTRUCT(BlueprintType)
struct FResourceState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource")
	float Maximum = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Resource")
	float CurrentValue = 0.0f;
	//Last prediction ID the server received that updated this resource.
	UPROPERTY()
	int32 PredictionID = 0;

	FResourceState() {}
	FResourceState(const float Max, const float Value) : Maximum(Max), CurrentValue(Value) {}
};

USTRUCT(BlueprintType)
struct FResourceInitInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bHasCustomMaximum = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (EditCondition = "bHasCustomMaximum", ClampMin = "0"))
	float CustomMaxValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bHasCustomInitial = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (EditCondition = "bHasCustomInitial", ClampMin = "0"))
	float CustomInitialValue = 0.0f;
};

//For modifiers to non-ability cost resource changes. Ability costs have their own modifiers, applied to specific abilities or the AbilityHandler.
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(FCombatModifier, FResourceDeltaModifier, UResource*, Resource, UObject*, Source, const float, InitialDelta);

//For notification of change in a resource's max or current value.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FResourceValueNotification, UResource*, Resource, UObject*, ChangeSource, const FResourceState&, PreviousState, const FResourceState&, NewState);

//For notification of a resource being added or removed.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceInstanceNotification, UResource*, Resource);