﻿#include "AbilityBuffFunctions.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "Resource.h"
#include "SaiyoraCombatInterface.h"

#pragma region Complex Ability Modifier

void FComplexAbilityMod::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	FBuffFunctionality::OnApply(ApplyEvent);

	TargetHandler = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetHandler()->GetOwner());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (ModifierFunction.IsNone())
	{
		return;
	}
	Modifier.BindUFunction(GetOwningBuff(), ModifierFunction);
	if (Modifier.IsBound())
	{
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
}

void FComplexAbilityMod::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	FBuffFunctionality::OnRemove(RemoveEvent);

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

void USimpleAbilityModifierFunction::SimpleAbilityModifier(UBuff* Buff, const TSubclassOf<UCombatAbility> AbilityClass,
	const ESimpleAbilityModType ModifierType, const FCombatModifier& Modifier)
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

void USimpleAbilityModifierFunction::SetModifierVars(const TSubclassOf<UCombatAbility> AbilityClass,
                                                     const ESimpleAbilityModType ModifierType, const FCombatModifier& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		if (IsValid(AbilityComponent))
		{
			TargetAbility = AbilityComponent->FindActiveAbility(AbilityClass);
			ModType = ModifierType;
			Mod = FCombatModifier(Modifier.Value, Modifier.Type, GetOwningBuff(), Modifier.bStackable);
		}
	}
}

void USimpleAbilityModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetAbility))
	{
		switch (ModType)
		{
			case ESimpleAbilityModType::ChargeCost :
				ModHandle = TargetAbility->AddChargeCostModifier(Mod);
				break;
			case ESimpleAbilityModType::MaxCharges :
				ModHandle = TargetAbility->AddMaxChargeModifier(Mod);
				break;
			case ESimpleAbilityModType::ChargesPerCooldown :
				ModHandle = TargetAbility->AddChargesPerCooldownModifier(Mod);
				break;
			default :
				break;
		}
	}
}

void USimpleAbilityModifierFunction::OnStack(const FBuffApplyEvent& ApplyEvent)
{
	if (Mod.bStackable)
	{
		if (IsValid(TargetAbility))
		{
			switch (ModType)
			{
			case ESimpleAbilityModType::ChargeCost :
				TargetAbility->UpdateChargeCostModifier(ModHandle, Mod);
				break;
			case ESimpleAbilityModType::MaxCharges :
				TargetAbility->UpdateMaxChargeModifier(ModHandle, Mod);
				break;
			case ESimpleAbilityModType::ChargesPerCooldown :
				TargetAbility->UpdateChargesPerCooldownModifier(ModHandle, Mod);
				break;
			default :
				break;
			}
		}
	}
}

void USimpleAbilityModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetAbility))
	{
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
}

#pragma endregion
#pragma region Ability Cost Modifier

void UAbilityCostModifierFunction::AbilityCostModifier(UBuff* Buff, const TSubclassOf<UResource> ResourceClass,
	const TSubclassOf<UCombatAbility> AbilityClass, const FCombatModifier& Modifier)
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

void UAbilityCostModifierFunction::SetModifierVars(const TSubclassOf<UResource> ResourceClass, const TSubclassOf<UCombatAbility> AbilityClass, const FCombatModifier& Modifier)
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

void UAbilityCostModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		if (IsValid(Ability))
		{
			if (IsValid(TargetAbility))
			{
				Handle = TargetAbility->AddResourceCostModifier(Resource, Mod);
			}
		}
		else
		{
			Handle = TargetHandler->AddGenericResourceCostModifier(Resource, Mod);
		}
	}
}

void UAbilityCostModifierFunction::OnStack(const FBuffApplyEvent& ApplyEvent)
{
	if (Mod.bStackable)
	{
		if (IsValid(TargetHandler))
		{
			if (IsValid(Ability))
			{
				if (IsValid(TargetAbility))
				{
					TargetAbility->UpdateResourceCostModifier(Resource, Handle, Mod);
				}
			}
			else
			{
				TargetHandler->UpdateGenericResourceCostModifier(Resource, Handle, Mod);
			}
		}
	}
}

void UAbilityCostModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		if (IsValid(Ability))
		{
			if (IsValid(TargetAbility))
			{
				TargetAbility->RemoveResourceCostModifier(Resource, Handle);
			}
		}
		else
		{
			TargetHandler->RemoveGenericResourceCostModifier(Resource, Handle);
		}
	}
}

#pragma endregion
#pragma region Ability Class Restriction

void UAbilityClassRestrictionFunction::AbilityClassRestrictions(UBuff* Buff, const TSet<TSubclassOf<UCombatAbility>>& AbilityClasses)
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

void UAbilityClassRestrictionFunction::SetRestrictionVars(const TSet<TSubclassOf<UCombatAbility>>& AbilityClasses)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		RestrictClasses = AbilityClasses;
	}
}

void UAbilityClassRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		for (const TSubclassOf<UCombatAbility> AbilityClass : RestrictClasses)
		{
			if (IsValid(AbilityClass))
			{
				TargetComponent->AddAbilityClassRestriction(GetOwningBuff(), AbilityClass);
			}
		}
	}
}

void UAbilityClassRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		for (const TSubclassOf<UCombatAbility> AbilityClass : RestrictClasses)
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

void UAbilityTagRestrictionFunction::AbilityTagRestrictions(UBuff* Buff, const FGameplayTagContainer& RestrictedTags)
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

void UAbilityTagRestrictionFunction::SetRestrictionVars(const FGameplayTagContainer& RestrictedTags)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		RestrictTags = RestrictedTags;
	}
}

void UAbilityTagRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
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
}

void UAbilityTagRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
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
}

#pragma endregion 
#pragma region Interrupt Restriction

void UInterruptRestrictionFunction::InterruptRestriction(UBuff* Buff, const FInterruptRestriction& Restriction)
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

void UInterruptRestrictionFunction::SetRestrictionVars(const FInterruptRestriction& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UInterruptRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->AddInterruptRestriction(Restrict);
	}
}

void UInterruptRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->RemoveInterruptRestriction(Restrict);
	}
}

#pragma endregion 