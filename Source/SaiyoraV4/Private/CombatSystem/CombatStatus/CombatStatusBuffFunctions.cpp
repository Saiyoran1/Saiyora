#include "CombatStatusBuffFunctions.h"
#include "Buff.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

void UPlaneSwapRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->AddPlaneSwapRestriction(GetOwningBuff());
}

void UPlaneSwapRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemovePlaneSwapRestriction(GetOwningBuff());
}