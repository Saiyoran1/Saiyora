#include "CombatStatusBuffFunctions.h"
#include "Buff.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

void UPlaneSwapRestrictionFunction::SetupBuffFunction()
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwningBuff()->GetAppliedTo());
}

void UPlaneSwapRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->AddPlaneSwapRestriction(GetOwningBuff());
	}
}

void UPlaneSwapRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->RemovePlaneSwapRestriction(GetOwningBuff());
	}
}