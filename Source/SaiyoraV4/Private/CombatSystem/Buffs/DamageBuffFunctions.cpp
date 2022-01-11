#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Damage Modifiers

void UDamageModifierFunction::DamageModifier(UBuff* Buff, EDamageModifierType const ModifierType,
                                             FDamageModCondition const& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EDamageModifierType::None || !Modifier.IsBound())
	{
		return;
	}
	UDamageModifierFunction* NewDamageModifierFunction = Cast<UDamageModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageModifierFunction))
	{
		return;
	}
	NewDamageModifierFunction->SetModifierVars(ModifierType, Modifier);
}

void UDamageModifierFunction::SetModifierVars(EDamageModifierType const ModifierType,
                                              FDamageModCondition const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		ModType = ModifierType;
		Mod = Modifier;
	}
}

void UDamageModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->AddIncomingDamageModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->AddOutgoingDamageModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->AddIncomingHealingModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->AddOutgoingHealingModifier(GetOwningBuff(), Mod);
			break;
		default :
			break;
		}
	}
}

void UDamageModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->RemoveIncomingDamageModifier(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->RemoveOutgoingDamageModifier(GetOwningBuff());
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->RemoveIncomingHealingModifier(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->RemoveOutgoingHealingModifier(GetOwningBuff());
			break;
		default :
			break;
		}
	}
}

#pragma endregion 
#pragma region Damage Restrictions

void UDamageRestrictionFunction::DamageRestriction(UBuff* Buff, EDamageModifierType const RestrictionType,
                                                   FDamageRestriction const& Restriction)
{
	if (!IsValid(Buff) || RestrictionType == EDamageModifierType::None || !Restriction.IsBound())
	{
		return;
	}
	UDamageRestrictionFunction* NewDamageRestrictionFunction = Cast<UDamageRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageRestrictionFunction))
	{
		return;
	}
	NewDamageRestrictionFunction->SetRestrictionVars(RestrictionType, Restriction);
}

void UDamageRestrictionFunction::SetRestrictionVars(EDamageModifierType const RestrictionType,
                                                    FDamageRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		RestrictType = RestrictionType;
		Restrict = Restriction;
	}
}

void UDamageRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->AddIncomingDamageRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->AddOutgoingDamageRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->AddIncomingHealingRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->AddOutgoingHealingRestriction(GetOwningBuff(), Restrict);
			break;
		default :
			break;
		}
	}
}

void UDamageRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->RemoveIncomingDamageRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->RemoveOutgoingDamageRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->RemoveIncomingHealingRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->RemoveOutgoingHealingRestriction(GetOwningBuff());
			break;
		default :
			break;
		}
	}
}

#pragma endregion
#pragma region Death Restriction

void UDeathRestrictionFunction::DeathRestriction(UBuff* Buff, FDeathRestriction const& Restriction)
{
	if (!IsValid(Buff) || !Restriction.IsBound())
	{
		return;
	}
	UDeathRestrictionFunction* NewDeathRestrictionFunction = Cast<UDeathRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDeathRestrictionFunction))
	{
		return;
	}
	NewDeathRestrictionFunction->SetRestrictionVars(Restriction);
}

void UDeathRestrictionFunction::SetRestrictionVars(FDeathRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UDeathRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->AddDeathRestriction(GetOwningBuff(), Restrict);
	}
}

void UDeathRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveDeathRestriction(GetOwningBuff());
	}
}

#pragma endregion