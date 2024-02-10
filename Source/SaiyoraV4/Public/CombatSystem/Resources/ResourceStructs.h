#pragma once
#include "CoreMinimal.h"
#include "CombatStructs.h"
#include "ResourceStructs.generated.h"

class UResource;

USTRUCT(BlueprintType)
struct FResourceState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Resource")
	float Minimum = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Resource")
	float Maximum = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Resource")
	float CurrentValue = 0.0f;
	//Last prediction ID the server received that updated this resource.
	UPROPERTY()
	int32 PredictionID = 0;

	FResourceState() {}
	FResourceState(const float Min, const float Max, const float Value) : Minimum(Min), Maximum(Max), CurrentValue(Value) {}
};

USTRUCT(BlueprintType)
struct FResourceInitInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bHasCustomMinimum = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (EditCondition = "bHasCustomMinimum", ClampMin = "0"))
	float CustomMinValue = 0.0f;
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

//For notification of change in a resource's max, min, or current value.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FResourceValueNotification, UResource*, Resource, UObject*, ChangeSource, const FResourceState&, PreviousState, const FResourceState&, NewState);

//For notification of a resource being added or removed.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceInstanceNotification, UResource*, Resource);