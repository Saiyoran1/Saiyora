#include "PlayerHUD/ActionBar.h"
#include "AbilityComponent.h"
#include "ActionSlot.h"
#include "SaiyoraPlayerCharacter.h"

void UActionBar::InitActionBar(const ESaiyoraPlane Plane, const ASaiyoraPlayerCharacter* OwningPlayer)
{
	if (!IsValid(OwningPlayer) || (Plane != ESaiyoraPlane::Ancient && Plane != ESaiyoraPlane::Modern)
		|| !IsValid(ActionSlotWidget))
	{
		return;
	}
	UAbilityComponent* OwnerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
	if (!IsValid(OwnerAbilityComp))
	{
		return;
	}
	AssignedPlane = Plane;
	for (int i = 0; i < OwningPlayer->MaxAbilityBinds; i++)
	{
		if (AssignedPlane == ESaiyoraPlane::Modern && i == 0)
		{
			continue;
		}
		UActionSlot* ActionSlot = CreateWidget<UActionSlot>(this, ActionSlotWidget);
		if (IsValid(ActionSlot))
		{
			ActionSlot->InitActionSlot(OwnerAbilityComp, AssignedPlane, i);
			ActionBox->AddChildToHorizontalBox(ActionSlot);
		}
	}
}