﻿#include "PlaneBuffFunctions.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"

void UPlaneSwapRestrictionFunction::PlaneSwapRestriction(UBuff* Buff, const FPlaneSwapRestriction& Restriction)
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

void UPlaneSwapRestrictionFunction::SetRestrictionVars(const FPlaneSwapRestriction& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UPlaneSwapRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->AddPlaneSwapRestriction(GetOwningBuff(), Restrict);
	}
}

void UPlaneSwapRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetComponent))
	{
		TargetComponent->RemovePlaneSwapRestriction(GetOwningBuff());
	}
}