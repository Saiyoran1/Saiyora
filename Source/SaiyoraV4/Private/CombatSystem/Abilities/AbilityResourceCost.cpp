// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityResourceCost.h"
#include "AbilityHandler.h"

void UAbilityResourceCost::Init(FAbilityCost const& InitInfo, TSubclassOf<UCombatAbility> const NewAbilityClass,
                                UAbilityHandler* NewHandler)
{
	if (!IsValid(NewHandler) || !IsValid(NewAbilityClass) || !IsValid(InitInfo.ResourceClass))
	{
		return;
	}
	Handler = NewHandler;
	AbilityClass = NewAbilityClass;
	ResourceClass = InitInfo.ResourceClass;
	UModifiableFloatValue::Init(InitInfo.Cost, !InitInfo.bStaticCost, true, 0.0f);
	BroadcastCallback.BindDynamic(this, &UAbilityResourceCost::BroadcastCostChanged);
	SubscribeToValueChanged(BroadcastCallback);
	CustomRecalculation.BindDynamic(this, &UAbilityResourceCost::RecalculateCost);
	SetRecalculationFunction(CustomRecalculation);
}

void UAbilityResourceCost::SubscribeToCostChanged(FResourceCostCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		OnCostChanged.AddUnique(Callback);
	}
}

void UAbilityResourceCost::UnsubscribeFromCostChanged(FResourceCostCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		OnCostChanged.Remove(Callback);
	}
}

void UAbilityResourceCost::BroadcastCostChanged(float const Previous, float const New)
{
	OnCostChanged.Broadcast(ResourceClass, New);
}

float UAbilityResourceCost::RecalculateCost(TArray<FCombatModifier> const& SpecificMods, float const Base)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, Base);
	}
	TArray<FCombatModifier> Mods = SpecificMods;
	Handler->GetCostModifiers(ResourceClass, AbilityClass, Mods);
	return FCombatModifier::ApplyModifiers(Mods, Base);
}