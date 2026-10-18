#include "CrowdControlBuffFunctions.h"
#include "Buff.h"
#include "CrowdControlHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Crowd Control

void UCrowdControlFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->AddCrowdControl(GetOwningBuff(), CrowdControl);
}

void UCrowdControlFunction::OnChange(const FBuffApplyEvent& ApplyEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (ApplyEvent.ActionTaken == EBuffApplyAction::Refreshed
		|| ApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
	{
		TargetHandler->RefreshCrowdControl(GetOwningBuff());
	}
}

void UCrowdControlFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveCrowdControl(GetOwningBuff());
}

bool UCrowdControlFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UCrowdControlHandler* CcHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(ApplyEvent.AppliedTo);
	if (!IsValid(CcHandler) || CcHandler->IsImmuneToCc(CrowdControl))
	{
		OutFailReason = EBuffApplyFailReason::ImmuneToCc;
		return true;
	}
	return false;
}

#pragma endregion 
#pragma region Crowd Control Immunity

void UCrowdControlImmunityFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->AddImmunities(GetOwningBuff(), ImmuneCcs);
}

void UCrowdControlImmunityFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!GetOwningBuff()->GetAppliedTo()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveImmunities(GetOwningBuff());
}

#pragma endregion