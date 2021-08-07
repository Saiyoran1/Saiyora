#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "AbilityHandler.h"
#include "CombatAbility.h"

void FAbilityModCollection::Initialize(TSubclassOf<UCombatAbility> const AbilityClass, UAbilityHandler* Handler)
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
	/*MaxCharges.bModifiable = !DefaultAbility->GetHasStaticMaxCharges();
	MaxCharges.bCappedLow = true;
	MaxCharges.Minimum = 1;
	MaxCharges.BaseValue = DefaultAbility->GetDefaultMaxCharges();
	GenericMaxChargeModifiers = &GenericMaxChargeMods;
	FModifierCallback MaxChargeCallback;
	MaxChargeCallback.BindRaw(this, &FAbilityModCollection::RecalculateMaxCharges);
	MaxChargeHandle = GenericMaxChargeMods.BindToModsChanged(MaxChargeCallback);
	TArray<FCombatModifier> Mods;
	GenericMaxChargeModifiers->GetSummedModifiers(Mods);
	TArray<FCombatModifier> SpecificMods;
	MaxChargeModifiers.GetSummedModifiers(SpecificMods);
	Mods.Append(SpecificMods);
	MaxCharges.Value = FCombatModifier::ApplyModifiers(Mods, MaxCharges.BaseValue);
	Mods.Empty();
	SpecificMods.Empty();*/
	//TODO: Finish the rest of this initialization of modifiers.
	//End goal is that each AbilityClass that the UAbilityHandler receives a Modifier for or instantiates will have one of these structs.
	//The struct will hold a constantly updated set of values for all modifiable values concerning an ability class.
	//PlayerAbilityHandler will likely need to recreate this on owning clients.
	//The end goal is that it should be very easy to check if an ability is castable without having to calculate everything, since any time a modifier is added or removed, final values will be updated.
	//Another goal is just to work out a more generic system for modifying values that can be used across other components.
	/*
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
	*/
	//TODO: callbacks to update UI?
	GlobalCooldownLength = FCombatFloatValue(DefaultAbility->GetDefaultGlobalCooldownLength(), !DefaultAbility->HasStaticGlobalCooldown(), true, UAbilityHandler::MinimumGlobalCooldownLength);
	GlobalCooldownLength.AddDependency(&GenericGlobalCooldownLengthMods);
	GlobalCooldownLength.AddDependency(&GlobalCooldownModifiers);

	CooldownLength = FCombatFloatValue(DefaultAbility->GetDefaultCooldown(), !DefaultAbility->GetHasStaticCooldown(), true, UAbilityHandler::MinimumCooldownLength);
	CooldownLength.AddDependency(&GenericCooldownLengthMods);
	CooldownLength.AddDependency(&CooldownModifiers);

	CastLength = FCombatFloatValue(DefaultAbility->GetDefaultCastLength(), !DefaultAbility->HasStaticCastLength(), true, UAbilityHandler::MinimumCastLength);
	CastLength.AddDependency(&GenericCastLengthMods);
	CastLength.AddDependency(&CastLengthModifiers);

	TArray<FAbilityCost> Costs;
	DefaultAbility->GetDefaultAbilityCosts(Costs);
	for (FAbilityCost& Cost : Costs)
	{
		if (IsValid(Cost.ResourceClass))
		{
			FCombatFloatValue& CostValue = AbilityCosts.Add(Cost.ResourceClass, FCombatFloatValue(Cost.Cost, !Cost.bStaticCost, true, 0.0f));
			FModifierCollection* CostModPtr = GenericCostMods.Find(Cost.ResourceClass);
			if (CostModPtr)
			{
				CostValue.AddDependency(CostModPtr);
			}
			CostModPtr = CostModifiers.Find(Cost.ResourceClass);
			if (CostModPtr)
			{
				CostValue.AddDependency(CostModPtr);
			}
			//TODO: Add dependency on resource delta mods? Maybe simplify the amount of things modifying resource cost?
			//Right now its ability-specific resource-specific cost mods, resource-specific ability cost mods, resource-specific delta mods.
			//(X Ability cost Y more Mana)	(Mana is spent Y more by all abilities)	(All changes in Mana are multipied by Y)
		}
	}
}

void FAbilityModCollection::RecalculateMaxCharges()
{
	
}