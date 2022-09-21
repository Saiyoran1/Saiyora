#include "ThreatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

#pragma region Threat Modifier

void UThreatModifierFunction::ThreatModifier(UBuff* Buff, EThreatModifierType const ModifierType,
                                             FThreatModCondition const& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EThreatModifierType::None || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UThreatModifierFunction* NewThreatModifierFunction = Cast<UThreatModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewThreatModifierFunction))
	{
		return;
	}
	NewThreatModifierFunction->SetModifierVars(ModifierType, Modifier);
}

void UThreatModifierFunction::SetModifierVars(EThreatModifierType const ModifierType,
                                              FThreatModCondition const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		ModType = ModifierType;
		Mod = Modifier;
	}
}

void UThreatModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->AddIncomingThreatModifier(Mod);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->AddOutgoingThreatModifier(Mod);
			break;
		default :
			break;
		}
	}
}

void UThreatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->RemoveIncomingThreatModifier(Mod);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->RemoveOutgoingThreatModifier(Mod);
			break;
		default :
			break;
		}
	}
}

#pragma endregion
#pragma region Threat Restriction

void UThreatRestrictionFunction::ThreatRestriction(UBuff* Buff, EThreatModifierType const RestrictionType,
                                                   FThreatRestriction const& Restriction)
{
	if (!IsValid(Buff) || RestrictionType == EThreatModifierType::None || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UThreatRestrictionFunction* NewThreatRestrictionFunction = Cast<UThreatRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewThreatRestrictionFunction))
	{
		return;
	}
	NewThreatRestrictionFunction->SetRestrictionVars(RestrictionType, Restriction);
}

void UThreatRestrictionFunction::SetRestrictionVars(EThreatModifierType const RestrictionType,
                                                    FThreatRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		RestrictType = RestrictionType;
		Restrict = Restriction;
	}
}

void UThreatRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{	
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->AddIncomingThreatRestriction(Restrict);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->AddOutgoingThreatRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

void UThreatRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->RemoveIncomingThreatRestriction(Restrict);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->RemoveOutgoingThreatRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

#pragma endregion