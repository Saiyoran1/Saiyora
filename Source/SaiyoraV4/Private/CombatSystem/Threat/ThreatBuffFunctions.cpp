#include "ThreatBuffFunctions.h"
#include "Buff.h"
#include "CombatStatusComponent.h"
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
#pragma region Fade

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

bool UFadeFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UThreatHandler* Handler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ApplyEvent.AppliedTo);
	if (!IsValid(Handler) || !Handler->CanBeInThreatTable())
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	return false;
}

#pragma endregion
#pragma region Fixate

void UFixateFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	FixateTarget = TryGetFixateTarget(ApplyEvent);
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

bool UFixateFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UThreatHandler* Handler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ApplyEvent.AppliedTo);
	if (!IsValid(Handler) || !Handler->HasThreatTable())
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	//Don't do the check for a valid target if this is a stack/refresh.
	//We don't want to open up the opportunity to change targets in the middle of the buff's lifetime.
	if (IsValid(ApplyEvent.AffectedBuff))
	{
		return false;
	}
	//Check that we have a valid target for fixating. If not, don't apply the buff.
	const AActor* Target = TryGetFixateTarget(ApplyEvent);
	if (!IsValid(Target))
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	return false;
}

AActor* UFixateFunction::TryGetFixateTarget(const FBuffApplyEvent& ApplyEvent) const
{
	AActor* Target = nullptr;
	//Look for a combat param providing the fixate target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "FixateTarget", TargetAsObject))
	{
		Target = Cast<AActor>(TargetAsObject);
	}
	//If the fixate target wasn't found in the buff params then default to the actor applying the buff.
	if (!IsValid(Target))
	{
		Target = ApplyEvent.AppliedBy;
	}
	//At this point, if we didn't find a target, just give up.
	if (!IsValid(Target))
	{
		return nullptr;
	}
	//Make sure the target is viable for fixation. Uses the same checks as the Threat Handler uses when applying fixates.
	if (!UThreatHandler::CanFixate(ApplyEvent.AppliedTo, Target))
	{
		return nullptr;
	}
	return Target;
}

#pragma endregion
#pragma region Blind

void UBlindFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	BlindTarget = TryGetBlindTarget(ApplyEvent);
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

bool UBlindFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UThreatHandler* Handler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ApplyEvent.AppliedTo);
	if (!IsValid(Handler) || !Handler->HasThreatTable())
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	//Don't do the check for a valid target if this is a stack/refresh.
	//We don't want to open up the opportunity to change targets in the middle of the buff's lifetime.
	if (IsValid(ApplyEvent.AffectedBuff))
	{
		return false;
	}
	//Check that we have a valid target for blinding. If not, don't apply the buff.
	const AActor* Target = TryGetBlindTarget(ApplyEvent);
	if (!IsValid(Target))
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	return false;
}

AActor* UBlindFunction::TryGetBlindTarget(const FBuffApplyEvent& ApplyEvent) const
{
	AActor* Target = nullptr;
	//Look for a combat param providing the blind target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "BlindTarget", TargetAsObject))
	{
		Target = Cast<AActor>(TargetAsObject);
	}
	//If the blind target wasn't found in the buff params then default to the actor applying the buff.
	if (!IsValid(Target))
	{
		Target = ApplyEvent.AppliedBy;
	}
	//At this point, if we didn't find a target, just give up.
	if (!IsValid(Target))
	{
		return nullptr;
	}
	//Make sure the target is viable for being affected by the blind. Uses the same checks as the Threat Handler uses when applying blinds.
	if (!UThreatHandler::CanBlind(ApplyEvent.AppliedTo, Target))
	{
		return nullptr;
	}
	return Target;
}

#pragma endregion
#pragma region Misdirect

void UMisdirectFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	MisdirectTarget = TryGetMisdirectTarget(ApplyEvent);
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

bool UMisdirectFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UThreatHandler* Handler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ApplyEvent.AppliedTo);
	if (!IsValid(Handler) || !Handler->CanBeInThreatTable())
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	//Don't do the check for a valid target if this is a stack/refresh.
	//We don't want to open up the opportunity to change targets in the middle of the buff's lifetime.
	if (IsValid(ApplyEvent.AffectedBuff))
	{
		return false;
	}
	//Check that we have a valid target for misdirecting. If not, don't apply the buff.
	const AActor* Target = TryGetMisdirectTarget(ApplyEvent);
	if (!IsValid(Target))
	{
		OutFailReason = EBuffApplyFailReason::InvalidThreatEvent;
		return true;
	}
	return false;
}

AActor* UMisdirectFunction::TryGetMisdirectTarget(const FBuffApplyEvent& ApplyEvent) const
{
	AActor* Target = nullptr;
	//Look for a combat param providing the misdirect target.
	UObject* TargetAsObject = nullptr;
	if (USaiyoraCombatLibrary::GetObjectParam(ApplyEvent.CombatParams, "MisdirectTarget", TargetAsObject))
	{
		Target = Cast<AActor>(TargetAsObject);
	}
	//If we didn't have a combat param, just go ahead and give up.
	if (!IsValid(Target))
	{
		return nullptr;
	}
	//Make sure the target is viable for being misdirected to. Uses the same checks as the Threat Handler uses when applying misdirected threat.
	if (!UThreatHandler::CanMisdirectThreat(ApplyEvent.AppliedTo, Target))
	{
		return nullptr;
	}
	return Target;
}

#pragma endregion
