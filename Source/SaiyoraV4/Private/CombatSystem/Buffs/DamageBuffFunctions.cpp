#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"

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
	ModType = ModifierType;
	Mod = Modifier;
}

void UDamageModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
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

void UDamageModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
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
