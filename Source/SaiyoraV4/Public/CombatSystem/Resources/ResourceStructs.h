// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraStructs.h"
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

//For modifiers to resource generation.
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(FCombatModifier, FResourceGainModifier, UResource*, Resource, UObject*, Source, float const, InitialDelta);

//For notification of resource expenditure and generation.
DECLARE_DYNAMIC_DELEGATE_FourParams(FResourceValueCallback, UResource*, Resource, UObject*, ChangeSource, FResourceState const&, PreviousState, FResourceState const&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FResourceValueNotification, UResource*, Resource, UObject*, ChangeSource, FResourceState const&, PreviousState, FResourceState const&, NewState);

//For notification of a resource being instantiated.
DECLARE_DYNAMIC_DELEGATE_OneParam(FResourceInstanceCallback, UResource*, Resource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceInstanceNotification, UResource*, Resource);

//Currently just used by UAbilityCost to broadcast changes to an ability's resource costs.
DECLARE_DYNAMIC_DELEGATE_TwoParams(FResourceCostCallback, TSubclassOf<UResource>, ResourceClass, float const, NewCost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FResourceCostNotification, TSubclassOf<UResource>, ResourceClass, float const, NewCost);