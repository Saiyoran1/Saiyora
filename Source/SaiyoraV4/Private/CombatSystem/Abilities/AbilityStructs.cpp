#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "AbilityHandler.h"
#include "CombatAbility.h"
#include "StatHandler.h"

FReplicableAbilityCost::FReplicableAbilityCost()
{
	Cost = 0.0f;
	ResourceClass = nullptr;
}

FReplicableAbilityCost::FReplicableAbilityCost(TSubclassOf<UResource> const NewResourceClass, float const NewCost)
{
	ResourceClass = NewResourceClass;
	Cost = NewCost;
}

void FAbilityResourceCost::Initialize(TSubclassOf<UResource> const NewResourceClass,
                                      TSubclassOf<UCombatAbility> const NewAbilityClass, UAbilityHandler* NewHandler)
{
	if (!IsValid(NewResourceClass) || !IsValid(NewAbilityClass) || !IsValid(NewHandler))
	{
		return;
	}
	Handler = NewHandler;
	ResourceClass = NewResourceClass;
	AbilityClass = NewAbilityClass;
	UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
	if (!IsValid(DefaultAbility))
	{
		return;
	}
	FAbilityCost const DefaultCost = DefaultAbility->GetDefaultAbilityCost(ResourceClass);
	Cost = NewObject<UModifiableFloatValue>(NewHandler->GetOwner());
	if (IsValid(Cost))
	{
		Cost->Init(DefaultCost.Cost, !DefaultCost.bStaticCost, true, 0.0f);
		FFloatValueRecalculation CostCalculation;
		CostCalculation.BindDynamic(this, &FAbilityResourceCost::RecalculateCost);
		Cost->SetRecalculationFunction(CostCalculation);
	}
}

float FAbilityResourceCost::RecalculateCost(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods = SpecificMods;
	Handler->GetCostModifiers(ResourceClass, AbilityClass, Mods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}