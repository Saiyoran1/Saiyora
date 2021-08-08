#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "AbilityHandler.h"
#include "CombatAbility.h"
#include "StatHandler.h"

float FAbilityValues::RecalculateGcdLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
	if (!IsValid(Handler))
	{
		return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->GenericGlobalCooldownModifiers)
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
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->GenericCooldownLengthModifiers)
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
	for (TTuple<int32, FAbilityModCondition> const& Condition : Handler->GenericCastLengthModifiers)
	{
		if (Condition.Value.IsBound())
		{
			Mods.Add(Condition.Value.Execute(AbilityClass));	
		}
	}
	Mods.Append(SpecificMods);
	return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void FAbilityValues::Initialize(TSubclassOf<UCombatAbility> const AbilityClass, UAbilityHandler* Handler)
{
	if (bInitialized == true)
	{
		return;
	}
	if (!IsValid(AbilityClass) || !IsValid(Handler))
	{
		return;
	}
	UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
	if (!IsValid(DefaultAbility))
	{
		return;
	}
	this->AbilityClass = AbilityClass;
	this->Handler = Handler;
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

	//TODO: Finish the rest of this initialization of modifiers.
	//End goal is that each AbilityClass that the UAbilityHandler receives a Modifier for or instantiates will have one of these structs.
	//The struct will hold a constantly updated set of all modifiable values concerning an ability class.
	//PlayerAbilityHandler will likely need to recreate this on owning clients.
	//The end goal is that it should be very easy to check if an ability is castable without having to calculate everything, since any time a modifier is added or removed, final values will be updated.
	//TODO: callbacks to update UI?
	//TODO: Costs???
	//TODO: CombatIntValue
}
