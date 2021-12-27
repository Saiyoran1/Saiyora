#include "AbilityBuffFunctions.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"

#pragma region Complex Ability Modifier

void UComplexAbilityModifierFunction::ComplexAbilityModifier(UBuff* Buff, EComplexAbilityModType const ModifierType, FAbilityModCondition const& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EComplexAbilityModType::None || !Modifier.IsBound())
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
	};
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
	if (!IsValid(Buff) || !IsValid(AbilityClass) || ModifierType == ESimpleAbilityModType::None || Modifier.Type == EModifierType::Invalid)
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