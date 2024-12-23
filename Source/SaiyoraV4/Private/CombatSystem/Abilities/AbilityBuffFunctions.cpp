#include "AbilityBuffFunctions.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region Complex Ability Modifier

void UComplexAbilityModifierFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetHandler()->GetOwner());
	Modifier.BindUFunction(GetOwningBuff(), ModifierFunctionName);
}

void UComplexAbilityModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler) || !Modifier.IsBound())
	{
		return;
	}
	switch (ModifierType)
	{
	case EComplexAbilityModType::CastLength :
		TargetHandler->AddCastLengthModifier(Modifier);
		break;
	case EComplexAbilityModType::CooldownLength :
		TargetHandler->AddCooldownModifier(Modifier);
		break;
	case EComplexAbilityModType::GlobalCooldownLength :
		TargetHandler->AddGlobalCooldownModifier(Modifier);
		break;
	default :
		break;
	}
}

void UComplexAbilityModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler) || !Modifier.IsBound())
	{
		return;
	}
	switch (ModifierType)
	{
	case EComplexAbilityModType::CastLength :
		TargetHandler->RemoveCastLengthModifier(Modifier);
		break;
	case EComplexAbilityModType::CooldownLength :
		TargetHandler->RemoveCooldownModifier(Modifier);
		break;
	case EComplexAbilityModType::GlobalCooldownLength :
		TargetHandler->RemoveGlobalCooldownModifier(Modifier);
		break;
	default :
		break;
	}
}

TArray<FName> UComplexAbilityModifierFunction::GetComplexAbilityModFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleModifierFunction"));
}

#pragma endregion 
#pragma region Simple Ability Modifier

void USimpleAbilityModifierFunction::SetupBuffFunction()
{
	const UAbilityComponent* AbilityHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (IsValid(AbilityHandler))
	{
		TargetAbility = AbilityHandler->FindActiveAbility(AbilityClass);
	}
}

void USimpleAbilityModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetAbility))
	{
		return;
	}
	switch (ModType)
	{
		case ESimpleAbilityModType::ChargeCost :
			ModHandle = TargetAbility->AddChargeCostModifier(Modifier);
			break;
		case ESimpleAbilityModType::MaxCharges :
			ModHandle = TargetAbility->AddMaxChargeModifier(Modifier);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			ModHandle = TargetAbility->AddChargesPerCooldownModifier(Modifier);
			break;
		default :
			break;
	}
}

void USimpleAbilityModifierFunction::OnChange(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetAbility))
	{
		return;
	}
	if (Modifier.bStackable && ApplyEvent.PreviousStacks != ApplyEvent.NewStacks)
	{
		switch (ModType)
		{
		case ESimpleAbilityModType::ChargeCost :
			TargetAbility->UpdateChargeCostModifier(ModHandle, Modifier);
			break;
		case ESimpleAbilityModType::MaxCharges :
			TargetAbility->UpdateMaxChargeModifier(ModHandle, Modifier);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			TargetAbility->UpdateChargesPerCooldownModifier(ModHandle, Modifier);
			break;
		default :
			break;
		}
	}
}

void USimpleAbilityModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetAbility))
	{
		return;
	}
	switch (ModType)
	{
		case ESimpleAbilityModType::ChargeCost :
			TargetAbility->RemoveChargeCostModifier(ModHandle);
			break;
		case ESimpleAbilityModType::MaxCharges :
			TargetAbility->RemoveMaxChargeModifier(ModHandle);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			TargetAbility->RemoveChargesPerCooldownModifier(ModHandle);
			break;
		default :
			break;
	}
}

#pragma endregion
#pragma region Ability Cost Modifier

void UAbilityCostModifierFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (IsValid(TargetHandler) && bSpecificAbility && IsValid(AbilityClass))
	{
		TargetAbility = TargetHandler->FindActiveAbility(AbilityClass);
	}
}

void UAbilityCostModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (bSpecificAbility && IsValid(AbilityClass))
	{
		if (IsValid(TargetAbility))
		{
			Handle = TargetAbility->AddResourceCostModifier(ResourceClass, Modifier);
		}
	}
	else
	{
		Handle = TargetHandler->AddGenericResourceCostModifier(ResourceClass, Modifier);
	}
}

void UAbilityCostModifierFunction::OnChange(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (Modifier.bStackable && ApplyEvent.PreviousStacks != ApplyEvent.NewStacks)
	{
		if (bSpecificAbility && IsValid(AbilityClass))
		{
			if (IsValid(TargetAbility))
			{
				TargetAbility->UpdateResourceCostModifier(ResourceClass, Handle, Modifier);
			}
		}
		else
		{
			TargetHandler->UpdateGenericResourceCostModifier(ResourceClass, Handle, Modifier);
		}
	}
}

void UAbilityCostModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (bSpecificAbility && IsValid(AbilityClass))
	{
		if (IsValid(TargetAbility))
		{
			TargetAbility->RemoveResourceCostModifier(ResourceClass, Handle);
		}
	}
	else
	{
		TargetHandler->RemoveGenericResourceCostModifier(ResourceClass, Handle);
	}
}

#pragma endregion
#pragma region Ability Class Restriction

void UAbilityClassRestrictionFunction::SetupBuffFunction()
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
}

void UAbilityClassRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetComponent))
	{
		return;
	}
	for (const TSubclassOf<UCombatAbility> AbilityClass : RestrictClasses)
	{
		if (IsValid(AbilityClass))
		{
			TargetComponent->AddAbilityClassRestriction(GetOwningBuff(), AbilityClass);
		}
	}
}

void UAbilityClassRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetComponent))
	{
		return;
	}
	for (const TSubclassOf<UCombatAbility> AbilityClass : RestrictClasses)
	{
		if (IsValid(AbilityClass))
		{
			TargetComponent->RemoveAbilityClassRestriction(GetOwningBuff(), AbilityClass);
		}
	}
}

#pragma endregion
#pragma region Ability Tag Restriction

void UAbilityTagRestrictionFunction::SetupBuffFunction()
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
}

void UAbilityTagRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetComponent))
	{
		return;
	}
	TArray<FGameplayTag> Tags;
	RestrictTags.GetGameplayTagArray(Tags);
	for (const FGameplayTag Tag : Tags)
	{
		if (Tag.IsValid())
		{
			TargetComponent->AddAbilityTagRestriction(GetOwningBuff(), Tag);
		}
	}
}

void UAbilityTagRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetComponent))
	{
		return;
	}
	TArray<FGameplayTag> Tags;
	RestrictTags.GetGameplayTagArray(Tags);
	for (const FGameplayTag Tag : Tags)
	{
		if (Tag.IsValid())
		{
			TargetComponent->RemoveAbilityTagRestriction(GetOwningBuff(), Tag);
		}
	}
}

#pragma endregion 
#pragma region Interrupt Restriction

void UInterruptRestrictionFunction::SetupBuffFunction()
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	Restriction.BindUFunction(GetOwningBuff(), RestrictionFunctionName);
}

void UInterruptRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent) && Restriction.IsBound())
	{
		TargetComponent->AddInterruptRestriction(Restriction);
	}
}

void UInterruptRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent) && Restriction.IsBound())
	{
		TargetComponent->RemoveInterruptRestriction(Restriction);
	}
}

TArray<FName> UInterruptRestrictionFunction::GetInterruptRestrictionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleRestrictionFunction"));
}

#pragma endregion 
