#include "ResourceBuffFunctions.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Resource Delta Modifier

UResourceDeltaModifierFunction::UResourceDeltaModifierFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UResourceDeltaModifierFunction, Modifier));
}

void UResourceDeltaModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	const UResourceHandler* TargetHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetResource = TargetHandler->FindActiveResource(ResourceClass);
	if (!IsValid(TargetResource))
	{
		return;
	}
	TargetResource->AddResourceDeltaModifier(Modifier);
}

void UResourceDeltaModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetResource))
	{
		return;
	}
	TargetResource->RemoveResourceDeltaModifier(Modifier);
}

#pragma endregion
