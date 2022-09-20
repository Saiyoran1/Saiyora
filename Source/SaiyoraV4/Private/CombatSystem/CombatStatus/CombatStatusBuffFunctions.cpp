#include "CombatStatusBuffFunctions.h"
#include "Buff.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

void UPlaneSwapRestrictionFunction::PlaneSwapRestriction(UBuff* Buff)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UPlaneSwapRestrictionFunction* NewPlaneSwapRestrictionFunction = Cast<UPlaneSwapRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewPlaneSwapRestrictionFunction))
	{
		return;
	}
	NewPlaneSwapRestrictionFunction->SetRestrictionVars();
}

void UPlaneSwapRestrictionFunction::SetRestrictionVars()
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwningBuff()->GetAppliedTo());
	}
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