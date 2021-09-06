// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraObjects.h"
#include "AbilityStructs.h"
#include "AbilityResourceCost.generated.h"

UCLASS()
class SAIYORAV4_API UAbilityResourceCost : public UModifiableFloatValue
{
	GENERATED_BODY()
private:
	TSubclassOf<class UResource> ResourceClass;
	TSubclassOf<class UCombatAbility> AbilityClass;
	UPROPERTY()
	class UAbilityHandler* Handler;
	FFloatValueCallback BroadcastCallback;
	UFUNCTION()
	void BroadcastCostChanged(float const Previous, float const New);
	FResourceCostNotification OnCostChanged;
	FFloatValueRecalculation CustomRecalculation;
	UFUNCTION()
	float RecalculateCost(TArray<FCombatModifier> const& SpecificMods, float const Base);
public:
	void Init(FAbilityCost const& InitInfo, TSubclassOf<UCombatAbility> const NewAbilityClass, UAbilityHandler* NewHandler);
	void SubscribeToCostChanged(FResourceCostCallback const& Callback);
	void UnsubscribeFromCostChanged(FResourceCostCallback const& Callback);
};
