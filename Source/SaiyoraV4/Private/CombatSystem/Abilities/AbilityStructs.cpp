#include "AbilityStructs.h"

#include "AbilityHandler.h"
#include "CombatAbility.h"

void FAbilityModCollection::Initialize(TSubclassOf<UCombatAbility> const AbilityClass,
                                       FModifierCollection& GenericMaxChargeMods, FModifierCollection& GenericChargesPerCastMods,
                                       FModifierCollection& GenericChargesPerCooldownMods, FModifierCollection& GenericGlobalCooldownLengthMods,
                                       FModifierCollection& GenericCooldownLengthMods, FModifierCollection& GenericCastLengthMods,
                                       TMap<TSubclassOf<UResource>, FModifierCollection&> const& GenericCostMods)
{
	if (bInitialized == true)
	{
		return;
	}
	if (!IsValid(AbilityClass))
	{
		return;
	}
	UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
	if (!IsValid(DefaultAbility))
	{
		return;
	}
	MaxCharges.bModifiable = !DefaultAbility->GetHasStaticMaxCharges();
	MaxCharges.bCappedLow = true;
	MaxCharges.Minimum = 1;
	MaxCharges.BaseValue = DefaultAbility->GetDefaultMaxCharges();
	GenericMaxChargeModifiers = &GenericMaxChargeMods;
	FModifierCallback MaxChargeCallback;
	MaxChargeCallback.BindRaw(this, &FAbilityModCollection::RecalculateMaxCharges);
	MaxChargeHandle = GenericMaxChargeMods.BindToModsChanged(MaxChargeCallback);
	TArray<FCombatModifier> Mods;
	GenericMaxChargeModifiers->GetModifiers(Mods);
	TArray<FCombatModifier> SpecificMods;
	MaxChargeModifiers.GetModifiers(SpecificMods);
	Mods.Append(SpecificMods);
	MaxCharges.Value = FCombatModifier::CombineModifiers(Mods, MaxCharges.BaseValue);
	Mods.Empty();
	SpecificMods.Empty();
	//TODO: Finish the rest of this initialization of modifiers.
	//End goal is that each AbilityClass that the UAbilityHandler receives a Modifier for or instantiates will have one of these structs.
	//The struct will hold a constantly updated set of values for all modifiable values concerning an ability class.
	//PlayerAbilityHandler will likely need to recreate this on owning clients.
	//The end goal is that it should be very easy to check if an ability is castable without having to calculate everything, since any time a modifier is added or removed, final values will be updated.
	//Another goal is just to work out a more generic system for modifying values that can be used across other components.
	ChargesPerCast.bModifiable = !DefaultAbility->GetHasStaticChargeCost();
	ChargesPerCast.bCappedLow = true;
	ChargesPerCast.Minimum = 0;
	ChargesPerCast.BaseValue = DefaultAbility->GetDefaultChargeCost();
	GenericChargesPerCastModifiers = &GenericChargesPerCastMods;
	FModifierCallback ChargesPerCastCallback;
	ChargesPerCastCallback.BindRaw(this, &FAbilityModCollection::RecalculateChargesPerCast);
	ChargesPerCastHandle = GenericChargesPerCastMods.BindToModsChanged(ChargesPerCastCallback);

	ChargesPerCooldown.bModifiable = !DefaultAbility->GetHasStaticChargesPerCooldown();
	ChargesPerCooldown.bCappedLow = true;
	ChargesPerCooldown.Minimum = 0;
	ChargesPerCooldown.BaseValue = DefaultAbility->GetDefaultChargesPerCooldown();
	GenericChargesPerCooldownModifiers = &GenericChargesPerCooldownMods;
	FModifierCallback ChargesPerCooldownCallback;
	ChargesPerCooldownCallback.BindRaw(this, &FAbilityModCollection::RecalculateChargesPerCooldown);
	ChargesPerCooldownHandle = GenericChargesPerCooldownMods.BindToModsChanged(ChargesPerCooldownCallback);

	GlobalCooldownLength.bModifiable = !DefaultAbility->HasStaticGlobalCooldown();
	GlobalCooldownLength.bCappedLow = true;
	GlobalCooldownLength.Minimum = UAbilityHandler::MinimumGlobalCooldownLength;
	GlobalCooldownLength.BaseValue = DefaultAbility->GetDefaultGlobalCooldownLength();
	GenericGlobalCooldownModifiers = &GenericGlobalCooldownLengthMods;
	FModifierCallback GlobalCooldownCallback;
	GlobalCooldownCallback.BindRaw(this, &FAbilityModCollection::RecalculateGlobalCooldownLength);
	GlobalCooldownHandle = GenericGlobalCooldownLengthMods.BindToModsChanged(GlobalCooldownCallback);

	CooldownLength.bModifiable = !DefaultAbility->GetHasStaticCooldown();
	CooldownLength.bCappedLow = true;
	CooldownLength.Minimum = UAbilityHandler::MinimumCooldownLength;
	CooldownLength.BaseValue = DefaultAbility->GetDefaultCooldown();
	GenericCooldownModifiers = &GenericCooldownLengthMods;
	FModifierCallback CooldownCallback;
	CooldownCallback.BindRaw(this, &FAbilityModCollection::RecalculateCooldownLength);
	CooldownHandle = GenericCooldownLengthMods.BindToModsChanged(CooldownCallback);

	CastLength.bModifiable = !DefaultAbility->HasStaticCastLength();
	CastLength.bCappedLow = true;
	CastLength.Minimum = UAbilityHandler::MinimumCastLength;
	CastLength.BaseValue = DefaultAbility->GetDefaultCastLength();
	GenericCastLengthModifiers = &GenericCastLengthMods;
	FModifierCallback CastLengthCallback;
	CastLengthCallback.BindRaw(this, &FAbilityModCollection::RecalculateCastLength);
	CastLengthHandle = GenericCastLengthMods.BindToModsChanged(CastLengthCallback);

	TArray<FAbilityCost> Costs;
	DefaultAbility->GetDefaultAbilityCosts(Costs);
	for (FAbilityCost& Cost : Costs)
	{
		if (IsValid(Cost.ResourceClass))
		{
			FCombatFloatValue NewCost;
			NewCost.bModifiable = !Cost.bStaticCost;
			NewCost.bCappedLow = true;
			NewCost.Minimum = 0.0f;
			NewCost.BaseValue = Cost.Cost;
			AbilityCosts.Add(Cost.ResourceClass, NewCost);
		}
	}
}

void FAbilityModCollection::RecalculateMaxCharges(FCombatModifier const& GenericAddMod,
	FCombatModifier const& GenericMultMod)
{
	
}