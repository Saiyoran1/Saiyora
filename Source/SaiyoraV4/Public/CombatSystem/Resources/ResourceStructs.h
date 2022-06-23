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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float CustomMinValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bHasCustomMaximum = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float CustomMaxValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bHasCustomInitial = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float CustomInitialValue = 0.0f;
};

//For modifiers to non-Ability resource changes.
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(FCombatModifier, FResourceDeltaModifier, UResource*, Resource, UObject*, Source, const float, InitialDelta);

//For notification of resource expenditure and generation.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FResourceValueNotification, UResource*, Resource, UObject*, ChangeSource, const FResourceState&, PreviousState, const FResourceState&, NewState);

//For notification of a resource being instantiated.
DECLARE_DYNAMIC_DELEGATE_OneParam(FResourceInstanceCallback, UResource*, Resource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceInstanceNotification, UResource*, Resource);