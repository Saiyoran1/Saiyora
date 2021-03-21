// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraStructs.h"
#include "GameplayTagContainer.h"
#include "AbilityStructs.h"
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
	TSubclassOf<UResource> ResourceClass;
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

//For modifiers to resource expenditure and generation.
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(FCombatModifier, FResourceDeltaModifier, FGameplayTag const&, ResourceTag, UObject*, Source, FAbilityCost const&, InitialDelta);

//For notification of resource expenditure and generation.
DECLARE_DYNAMIC_DELEGATE_FourParams(FResourceValueCallback, FGameplayTag const&, ResourceTag, UObject*, ChangeSource, FResourceState const&, PreviousState, FResourceState const&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FResourceValueNotification, FGameplayTag const&, ResourceTag, UObject*, ChangeSource, FResourceState const&, PreviousState, FResourceState const&, NewState);

//For notification of modifiers added to a resource (for spells to recalculate their costs and support client prediction).
DECLARE_DYNAMIC_DELEGATE_OneParam(FResourceTagCallback, FGameplayTag const&, ResourceTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceTagNotification, FGameplayTag const&, ResourceTag);

//For notification of a resource being instantiated.
DECLARE_DYNAMIC_DELEGATE_OneParam(FResourceInstanceCallback, FGameplayTag const&, ResourceTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceInstanceNotification, FGameplayTag const&, ResourceTag);