#include "PlaneBuffFunctions.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"

void UPlaneSwapRestrictionFunction::PlaneSwapRestriction(UBuff* Buff, FPlaneSwapRestriction const& Restriction)
{
	if (!IsValid(Buff) || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UPlaneSwapRestrictionFunction* NewPlaneSwapRestrictionFunction = Cast<UPlaneSwapRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewPlaneSwapRestrictionFunction))
	{
		return;
	}
	NewPlaneSwapRestrictionFunction->SetRestrictionVars(Restriction);
}

void UPlaneSwapRestrictionFunction::SetRestrictionVars(FPlaneSwapRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UPlaneSwapRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->AddPlaneSwapRestriction(GetOwningBuff(), Restrict);
	}
}

void UPlaneSwapRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->RemovePlaneSwapRestriction(GetOwningBuff());
	}
}