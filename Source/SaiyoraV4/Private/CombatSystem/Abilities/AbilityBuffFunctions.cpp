#include "AbilityBuffFunctions.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"

#pragma region Complex Ability Modifier

void UComplexAbilityModifierFunction::ComplexAbilityModifier(UBuff* Buff, EComplexAbilityModType const ModifierType, FAbilityModCondition const& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EComplexAbilityModType::None || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UComplexAbilityModifierFunction* NewAbilityModifierFunction = Cast<UComplexAbilityModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityModifierFunction))
	{
		return;
	}
	NewAbilityModifierFunction->SetModifierVars(ModifierType, Modifier);
}

void UComplexAbilityModifierFunction::SetModifierVars(EComplexAbilityModType const ModifierType,
                                                      FAbilityModCondition const& Modifier)
{
	ModType = ModifierType;
	Mod = Modifier;
}

void UComplexAbilityModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
    if (!IsValid(TargetHandler))
    {
        return;
    }
	switch (ModType)
	{
		case EComplexAbilityModType::CastLength :
			TargetHandler->AddCastLengthModifier(GetOwningBuff(), Mod);
			break;
		case EComplexAbilityModType::CooldownLength :
			TargetHandler->AddCooldownModifier(GetOwningBuff(), Mod);
			break;
		case EComplexAbilityModType::GlobalCooldownLength :
			TargetHandler->AddGlobalCooldownModifier(GetOwningBuff(), Mod);
			break;
		default :
			break;
	}
}

void UComplexAbilityModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
			case EComplexAbilityModType::CastLength :
				TargetHandler->RemoveCastLengthModifier(GetOwningBuff());
				break;
			case EComplexAbilityModType::CooldownLength :
				TargetHandler->RemoveCooldownModifier(GetOwningBuff());
				break;
			case EComplexAbilityModType::GlobalCooldownLength :
				TargetHandler->RemoveGlobalCooldownModifier(GetOwningBuff());
				break;
			default :
				break;
		}
	}
}

#pragma endregion 
#pragma region Simple Ability Modifier

void USimpleAbilityModifierFunction::SimpleAbilityModifier(UBuff* Buff, TSubclassOf<UCombatAbility> const AbilityClass,
	ESimpleAbilityModType const ModifierType, FCombatModifier const& Modifier)
{
	if (!IsValid(Buff) || !IsValid(AbilityClass) || ModifierType == ESimpleAbilityModType::None || Modifier.Type == EModifierType::Invalid || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	USimpleAbilityModifierFunction* NewAbilityModifierFunction = Cast<USimpleAbilityModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityModifierFunction))
	{
		return;
	}
	NewAbilityModifierFunction->SetModifierVars(AbilityClass, ModifierType, Modifier);
}

void USimpleAbilityModifierFunction::SetModifierVars(TSubclassOf<UCombatAbility> const AbilityClass,
                                                     ESimpleAbilityModType const ModifierType, FCombatModifier const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		if (IsValid(AbilityComponent))
		{
			TargetAbility = AbilityComponent->FindActiveAbility(AbilityClass);
			ModType = ModifierType;
			Mod = FCombatModifier(Modifier.Value, Modifier.Type, GetOwningBuff(), Modifier.bStackable);
		}
	}
}

void USimpleAbilityModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetAbility))
	{
		switch (ModType)
		{
			case ESimpleAbilityModType::ChargeCost :
				TargetAbility->AddChargeCostModifier(Mod);
				break;
			case ESimpleAbilityModType::MaxCharges :
				TargetAbility->AddMaxChargeModifier(Mod);
				break;
			case ESimpleAbilityModType::ChargesPerCooldown :
				TargetAbility->AddChargesPerCooldownModifier(Mod);
				break;
			default :
				break;
		}
	}
}

void USimpleAbilityModifierFunction::OnStack(FBuffApplyEvent const& ApplyEvent)
{
	if (Mod.bStackable)
	{
		if (IsValid(TargetAbility))
		{
			switch (ModType)
			{
			case ESimpleAbilityModType::ChargeCost :
				TargetAbility->AddChargeCostModifier(Mod);
				break;
			case ESimpleAbilityModType::MaxCharges :
				TargetAbility->AddMaxChargeModifier(Mod);
				break;
			case ESimpleAbilityModType::ChargesPerCooldown :
				TargetAbility->AddChargesPerCooldownModifier(Mod);
				break;
			default :
				break;
			}
		}
	}
}

void USimpleAbilityModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetAbility))
	{
		switch (ModType)
		{
			case ESimpleAbilityModType::ChargeCost :
				TargetAbility->RemoveChargeCostModifier(GetOwningBuff());
				break;
			case ESimpleAbilityModType::MaxCharges :
				TargetAbility->RemoveMaxChargeModifier(GetOwningBuff());
				break;
			case ESimpleAbilityModType::ChargesPerCooldown :
				TargetAbility->RemoveChargesPerCooldownModifier(GetOwningBuff());
				break;
			default :
				break;
		}
	}
}

#pragma endregion
#pragma region Ability Cost Modifier

void UAbilityCostModifierFunction::AbilityCostModifier(UBuff* Buff, TSubclassOf<UResource> const ResourceClass,
	TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier)
{
	if (!IsValid(Buff) || !IsValid(ResourceClass) || Modifier.Type == EModifierType::Invalid || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UAbilityCostModifierFunction* NewAbilityModifierFunction = Cast<UAbilityCostModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityModifierFunction))
	{
		return;
	}
	NewAbilityModifierFunction->SetModifierVars(ResourceClass, AbilityClass, Modifier);
}

void UAbilityCostModifierFunction::SetModifierVars(TSubclassOf<UResource> const ResourceClass, TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		if (IsValid(TargetHandler))
		{
			Resource = ResourceClass;
			Ability = AbilityClass;
			Mod = FCombatModifier(Modifier.Value, Modifier.Type, GetOwningBuff(), Modifier.bStackable);
			if (IsValid(Ability))
			{
				TargetAbility = TargetHandler->FindActiveAbility(Ability);
			}
		}
	}
}

void UAbilityCostModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		if (IsValid(Ability))
		{
			if (IsValid(TargetAbility))
			{
				TargetAbility->AddResourceCostModifier(Resource, Mod);
			}
		}
		else
		{
			TargetHandler->AddGenericResourceCostModifier(Resource, Mod);
		}
	}
}

void UAbilityCostModifierFunction::OnStack(FBuffApplyEvent const& ApplyEvent)
{
	if (Mod.bStackable)
	{
		if (IsValid(TargetHandler))
		{
			if (IsValid(Ability))
			{
				if (IsValid(TargetAbility))
				{
					TargetAbility->AddResourceCostModifier(Resource, Mod);
				}
			}
			else
			{
				TargetHandler->AddGenericResourceCostModifier(Resource, Mod);
			}
		}
	}
}

void UAbilityCostModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		if (IsValid(Ability))
		{
			if (IsValid(TargetAbility))
			{
				TargetAbility->RemoveResourceCostModifier(Resource, GetOwningBuff());
			}
		}
		else
		{
			TargetHandler->RemoveGenericResourceCostModifier(Resource, GetOwningBuff());
		}
	}
}

#pragma endregion
#pragma region Ability Class Restriction

void UAbilityClassRestrictionFunction::AbilityClassRestrictions(UBuff* Buff, TSet<TSubclassOf<UCombatAbility>> const& AbilityClasses)
{
	if (!IsValid(Buff) || AbilityClasses.Num() == 0 || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UAbilityClassRestrictionFunction* NewAbilityRestrictionFunction = Cast<UAbilityClassRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityRestrictionFunction))
	{
		return;
	}
	NewAbilityRestrictionFunction->SetRestrictionVars(AbilityClasses);
}

void UAbilityClassRestrictionFunction::SetRestrictionVars(TSet<TSubclassOf<UCombatAbility>> const& AbilityClasses)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		RestrictClasses = AbilityClasses;
	}
}

void UAbilityClassRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		for (TSubclassOf<UCombatAbility> const AbilityClass : RestrictClasses)
		{
			if (IsValid(AbilityClass))
			{
				TargetComponent->AddAbilityClassRestriction(GetOwningBuff(), AbilityClass);
			}
		}
	}
}

void UAbilityClassRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		for (TSubclassOf<UCombatAbility> const AbilityClass : RestrictClasses)
		{
			if (IsValid(AbilityClass))
			{
				TargetComponent->RemoveAbilityClassRestriction(GetOwningBuff(), AbilityClass);
			}
		}
	}
}

#pragma endregion
#pragma region Ability Tag Restriction

void UAbilityTagRestrictionFunction::AbilityTagRestrictions(UBuff* Buff, FGameplayTagContainer const& RestrictedTags)
{
	if (!IsValid(Buff) || RestrictedTags.IsEmpty() || !RestrictedTags.IsValid() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UAbilityTagRestrictionFunction* NewAbilityRestrictionFunction = Cast<UAbilityTagRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityRestrictionFunction))
	{
		return;
	}
	NewAbilityRestrictionFunction->SetRestrictionVars(RestrictedTags);
}

void UAbilityTagRestrictionFunction::SetRestrictionVars(FGameplayTagContainer const& RestrictedTags)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		RestrictTags = RestrictedTags;
	}
}

void UAbilityTagRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TArray<FGameplayTag> Tags;
		RestrictTags.GetGameplayTagArray(Tags);
		for (FGameplayTag const Tag : Tags)
		{
			if (Tag.IsValid())
			{
				TargetComponent->AddAbilityTagRestriction(GetOwningBuff(), Tag);
			}
		}
	}
}

void UAbilityTagRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TArray<FGameplayTag> Tags;
		RestrictTags.GetGameplayTagArray(Tags);
		for (FGameplayTag const Tag : Tags)
		{
			if (Tag.IsValid())
			{
				TargetComponent->RemoveAbilityTagRestriction(GetOwningBuff(), Tag);
			}
		}
	}
}

#pragma endregion 
#pragma region Interrupt Restriction

void UInterruptRestrictionFunction::InterruptRestriction(UBuff* Buff, FInterruptRestriction const& Restriction)
{
	if (!IsValid(Buff) || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UInterruptRestrictionFunction* NewInterruptRestrictionFunction = Cast<UInterruptRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewInterruptRestrictionFunction))
	{
		return;
	}
	NewInterruptRestrictionFunction->SetRestrictionVars(Restriction);
}

void UInterruptRestrictionFunction::SetRestrictionVars(FInterruptRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UInterruptRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->AddInterruptRestriction(GetOwningBuff(), Restrict);
	}
}

void UInterruptRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->RemoveInterruptRestriction(GetOwningBuff());
	}
}

#pragma endregion 