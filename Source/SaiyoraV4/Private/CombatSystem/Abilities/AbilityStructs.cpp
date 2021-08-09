#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "AbilityHandler.h"
#include "CombatAbility.h"
#include "StatHandler.h"

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
	Cost = FCombatFloatValue(DefaultCost.Cost, !DefaultCost.bStaticCost, true, 0.0f);
	FCombatValueRecalculation CostCalculation;
	CostCalculation.BindRaw(this, &FAbilityResourceCost::RecalculateCost);
	Cost.SetRecalculationFunction(CostCalculation);
}

float FAbilityResourceCost::RecalculateCost(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<int32, FAbilityResourceModCondition> const& Condition : Handler->ConditionalCostModifiers)
	{
		if (Condition.Value.IsBound())
		{
			Mods.Add(Condition.Value.Execute(AbilityClass, ResourceClass));
		}
	}
	Mods.Append(SpecificMods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

float FAbilityValues::RecalculateGcdLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->ConditionalGlobalCooldownModifiers)
	{
		if (Condition.Value.IsBound())
		{
			Mods.Add(Condition.Value.Execute(AbilityClass));
		}
	}
	Mods.Append(SpecificMods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

float FAbilityValues::RecalculateCooldownLength(TArray<FCombatModifier> const& SpecificMods,
	float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->ConditionalCooldownModifiers)
	{
		if (Condition.Value.IsBound())
		{
			Mods.Add(Condition.Value.Execute(AbilityClass));	
		}
	}
	Mods.Append(SpecificMods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

float FAbilityValues::RecalculateCastLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->ConditionalCastLengthModifiers)
	{
		if (Condition.Value.IsBound())
		{
			Mods.Add(Condition.Value.Execute(AbilityClass));	
		}
	}
	Mods.Append(SpecificMods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void FAbilityValues::Initialize(TSubclassOf<UCombatAbility> const NewAbilityClass, UAbilityHandler* NewHandler)
{
	if (bInitialized == true)
	{
		return;
	}
	if (!IsValid(NewAbilityClass) || !IsValid(NewHandler))
	{
		return;
	}
	UCombatAbility const* DefaultAbility = NewAbilityClass->GetDefaultObject<UCombatAbility>();
	if (!IsValid(DefaultAbility))
	{
		return;
	}
	AbilityClass = NewAbilityClass;
	Handler = NewHandler;
	GlobalCooldownLength = FCombatFloatValue(DefaultAbility->GetDefaultGlobalCooldownLength(), !DefaultAbility->HasStaticGlobalCooldown(), true, UAbilityHandler::MinimumGlobalCooldownLength);
	if (!DefaultAbility->HasStaticGlobalCooldown())
	{
		FCombatValueRecalculation GcdCalculation;
		GcdCalculation.BindRaw(this, &FAbilityValues::RecalculateGcdLength);
		GlobalCooldownLength.SetRecalculationFunction(GcdCalculation);
	}
	CooldownLength = FCombatFloatValue(DefaultAbility->GetDefaultCooldown(), !DefaultAbility->GetHasStaticCooldown(), true, UAbilityHandler::MinimumCooldownLength);
	if (!DefaultAbility->GetHasStaticCooldown())
	{
		FCombatValueRecalculation CooldownRecalculation;
		CooldownRecalculation.BindRaw(this, &FAbilityValues::RecalculateCooldownLength);
		CooldownLength.SetRecalculationFunction(CooldownRecalculation);
	}
	if (DefaultAbility->GetCastType() == EAbilityCastType::Channel)
	{
		CastLength = FCombatFloatValue(DefaultAbility->GetDefaultCastLength(), !DefaultAbility->HasStaticCastLength(), true, UAbilityHandler::MinimumCastLength);
		if (!DefaultAbility->HasStaticCastLength())
		{
			FCombatValueRecalculation CastLengthRecalculation;
			CastLengthRecalculation.BindRaw(this, &FAbilityValues::RecalculateCastLength);
			CastLength.SetRecalculationFunction(CastLengthRecalculation);
		}
	}
	MaxCharges = FCombatIntValue(DefaultAbility->GetDefaultMaxCharges(), !DefaultAbility->GetHasStaticMaxCharges(), true, 1);
	ChargeCost = FCombatIntValue(DefaultAbility->GetDefaultChargeCost(), !DefaultAbility->HasStaticChargeCost(), true, 0);
	ChargesPerCooldown = FCombatIntValue(DefaultAbility->GetDefaultChargesPerCooldown(), !DefaultAbility->GetHasStaticChargesPerCooldown(), true, 0);
	TArray<FAbilityCost> DefaultCosts;
	DefaultAbility->GetDefaultAbilityCosts(DefaultCosts);
	for (FAbilityCost const& Cost : DefaultCosts)
	{
		FAbilityResourceCost& ResourceCost = AbilityCosts.Add(Cost.ResourceClass);
		ResourceCost.Initialize(Cost.ResourceClass, AbilityClass, Handler);
	}
	//TODO: Finish the rest of this initialization of modifiers.
	//End goal is that each AbilityClass that the UAbilityHandler receives a Modifier for or instantiates will have one of these structs.
	//The struct will hold a constantly updated set of all modifiable values concerning an ability class.
	//PlayerAbilityHandler will likely need to recreate this on owning clients.
	//The end goal is that it should be very easy to check if an ability is castable without having to calculate everything, since any time a modifier is added or removed, final values will be updated.
	//TODO: callbacks to update UI?
}

int32 FAbilityValues::AddCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
	FAbilityResourceCost* Cost = AbilityCosts.Find(ResourceClass);
	if (!Cost)
	{
		return -1;
	}
	return Cost->Cost.AddModifier(Modifier);
}

void FAbilityValues::RemoveCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID)
{
	if (ModifierID == -1)
	{
		return;
	}
	FAbilityResourceCost* Cost = AbilityCosts.Find(ResourceClass);
	if (!Cost)
	{
		return;
	}
	Cost->Cost.RemoveModifier(ModifierID);
}

void FAbilityValues::UpdateCost(TSubclassOf<UResource> const ResourceClass)
{
	FAbilityResourceCost* Cost = AbilityCosts.Find(ResourceClass);
	if (!Cost)
	{
		return;
	}
	Cost->Cost.ForceRecalculation();
}

void FAbilityValues::UpdateAllCosts()
{
	for (TTuple<TSubclassOf<UResource>, FAbilityResourceCost>& Cost : AbilityCosts)
	{
		Cost.Value.Cost.ForceRecalculation();
	}
}

void FAbilityValues::GetCosts(TMap<TSubclassOf<UResource>, float>& OutCosts)
{
	for (TTuple<TSubclassOf<UResource>, FAbilityResourceCost>& Cost : AbilityCosts)
	{
		OutCosts.Add(Cost.Key, Cost.Value.Cost.GetValue());
	}
}
