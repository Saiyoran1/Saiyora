#include "ThreatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "ThreatHandler.h"

#pragma region Threat Modifier

UThreatModifierFunction::UThreatModifierFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UThreatModifierFunction, Modifier));
}

void UThreatModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->AddIncomingThreatModifier(Modifier);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->AddOutgoingThreatModifier(Modifier);
		break;
	default :
		break;
	}
}

void UThreatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->RemoveIncomingThreatModifier(Modifier);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->RemoveOutgoingThreatModifier(Modifier);
		break;
	default :
		break;
	}
}

#pragma endregion
#pragma region Threat Restriction

UThreatRestrictionFunction::UThreatRestrictionFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UThreatRestrictionFunction, Restriction));
}

void UThreatRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->AddIncomingThreatRestriction(Restriction);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->AddOutgoingThreatRestriction(Restriction);
		break;
	default :
		break;
	}
}

void UThreatRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->RemoveIncomingThreatRestriction(Restriction);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->RemoveOutgoingThreatRestriction(Restriction);
		break;
	default :
		break;
	}
}

#pragma endregion
#pragma region Threat Actions

void UFadeFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->AddFade(GetOwningBuff());
}

void UFadeFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;	
	}
	TargetHandler->RemoveFade(GetOwningBuff());
}

void UFixateFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	//Look for a combat param providing the fixate target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "FixateTarget", TargetAsObject))
	{
		FixateTarget = Cast<AActor>(TargetAsObject);
	}
	//If the fixate target wasn't found in the buff params then default to the actor applying the buff.
	if (!IsValid(FixateTarget))
	{
		FixateTarget = GetOwningBuff()->GetAppliedBy();
	}
	if (IsValid(FixateTarget))
	{
		TargetHandler->AddFixate(FixateTarget, GetOwningBuff());
	}
}

void UFixateFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveFixate(FixateTarget, GetOwningBuff());
}

void UBlindFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	//Look for a combat param providing the blind target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "BlindTarget", TargetAsObject))
	{
		BlindTarget = Cast<AActor>(TargetAsObject);
	}
	//If the blind target wasn't found in the buff params, then default to the actor applying the buff.
	if (!IsValid(BlindTarget))
	{
		BlindTarget = GetOwningBuff()->GetAppliedBy();
	}
	if (IsValid(BlindTarget))
	{
		TargetHandler->AddBlind(BlindTarget, GetOwningBuff());
	}
}

void UBlindFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveBlind(BlindTarget, GetOwningBuff());
}

void UMisdirectFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	//Look for a combat param providing the misdirect target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "MisdirectTarget", TargetAsObject))
	{
		MisdirectTarget = Cast<AActor>(TargetAsObject);
	}
	if (IsValid(MisdirectTarget))
	{
		TargetHandler->AddMisdirect(MisdirectTarget, GetOwningBuff());
	}
}

void UMisdirectFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveMisdirect(GetOwningBuff());
}

#pragma endregion
