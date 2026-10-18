#include "AbilityBuffFunctions.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region Complex Ability Modifier

UComplexAbilityModifierFunction::UComplexAbilityModifierFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UComplexAbilityModifierFunction, Modifier));
}

void UComplexAbilityModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
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
	if (!IsValid(TargetHandler))
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

#pragma endregion 
#pragma region Simple Ability Modifier

void USimpleAbilityModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	const UAbilityComponent* AbilityHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (IsValid(AbilityHandler))
	{
		TargetAbility = AbilityHandler->FindActiveAbility(AbilityClass);
	}
	if (!IsValid(TargetAbility))
	{
		return;
	}
	Modifier.BuffSource = GetOwningBuff();
	switch (ModType)
	{
		case ESimpleAbilityModType::ChargeCost :
			Handle = TargetAbility->AddChargeCostModifier(Modifier);
			break;
		case ESimpleAbilityModType::MaxCharges :
			Handle = TargetAbility->AddMaxChargeModifier(Modifier);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			Handle = TargetAbility->AddChargesPerCooldownModifier(Modifier);
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
			TargetAbility->UpdateChargeCostModifier(Handle, Modifier);
			break;
		case ESimpleAbilityModType::MaxCharges :
			TargetAbility->UpdateMaxChargeModifier(Handle, Modifier);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			TargetAbility->UpdateChargesPerCooldownModifier(Handle, Modifier);
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
			TargetAbility->RemoveChargeCostModifier(Handle);
			break;
		case ESimpleAbilityModType::MaxCharges :
			TargetAbility->RemoveMaxChargeModifier(Handle);
			break;
		case ESimpleAbilityModType::ChargesPerCooldown :
			TargetAbility->RemoveChargesPerCooldownModifier(Handle);
			break;
		default :
			break;
	}
}

#pragma endregion
#pragma region Ability Cost Modifier

void UAbilityCostModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	Modifier.BuffSource = GetOwningBuff();
	if (bSpecificAbility && IsValid(AbilityClass))
	{
		TargetAbility = TargetHandler->FindActiveAbility(AbilityClass);
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

void UAbilityClassRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
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

void UAbilityTagRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
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

UInterruptRestrictionFunction::UInterruptRestrictionFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UInterruptRestrictionFunction, Restriction));
}

void UInterruptRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetComponent))
	{
		return;
	}
	TargetComponent->AddInterruptRestriction(Restriction);
}

void UInterruptRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetComponent))
	{
		return;
	}
	TargetComponent->RemoveInterruptRestriction(Restriction);
}

#pragma endregion 