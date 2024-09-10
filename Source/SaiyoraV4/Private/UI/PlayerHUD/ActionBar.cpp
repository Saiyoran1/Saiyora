#include "PlayerHUD/ActionBar.h"
#include "AbilityComponent.h"
#include "ActionSlot.h"
#include "Border.h"
#include "CombatStatusComponent.h"
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
			ActionSlots.Add(ActionSlot);
			ActionSlot->SetActive(PlaneSwapAlpha);
			ActionBox->AddChildToHorizontalBox(ActionSlot);
		}
	}
	UCombatStatusComponent* OwnerCombatComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(OwningPlayer);
	if (!IsValid(OwnerCombatComp))
	{
		return;
	}
	OwnerCombatComp->OnPlaneSwapped.AddDynamic(this, &UActionBar::OnPlaneSwapped);
	OnPlaneSwapped(ESaiyoraPlane::Neither, OwnerCombatComp->GetCurrentPlane(), nullptr);
}

void UActionBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float PreviousAlpha = PlaneSwapAlpha;
	if (bInCorrectPlane && PlaneSwapAlpha < 1.0f)
	{
		PlaneSwapAlpha = FMath::Clamp(PlaneSwapAlpha + (InDeltaTime / PlaneSwapAnimDuration), 0.0f, 1.0f);
	}
	else if (!bInCorrectPlane && PlaneSwapAlpha > 0.0f)
	{
		PlaneSwapAlpha = FMath::Clamp(PlaneSwapAlpha - (InDeltaTime / PlaneSwapAnimDuration), 0.0f, 1.0f);
	}
	if (PreviousAlpha != PlaneSwapAlpha)
	{
		const float ActualAlpha = IsValid(PlaneSwapAnimCurve) ? PlaneSwapAnimCurve->GetFloatValue(PlaneSwapAlpha) : PlaneSwapAlpha;
		SetRenderScale(FVector2D(FMath::Lerp(MinScale, MaxScale, ActualAlpha)));
		if (IsValid(ActionBorder))
		{
			ActionBorder->SetContentColorAndOpacity(FMath::Lerp(DesaturatedTint, FLinearColor::White, ActualAlpha));
		}
		for (UActionSlot* ActionSlot : ActionSlots)
		{
			if (IsValid(ActionSlot))
			{
				ActionSlot->SetActive(PlaneSwapAlpha);
			}
		}
	}
}

void UActionBar::OnPlaneSwapped(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source)
{
	bInCorrectPlane = NewPlane == AssignedPlane;
}

void UActionBar::AddAbilityProc(UBuff* SourceBuff, const TSubclassOf<UCombatAbility> AbilityClass)
{
	for (UActionSlot* ActionSlot : ActionSlots)
	{
		if (ActionSlot->GetAbilityClass() == AbilityClass)
		{
			ActionSlot->ApplyProc(SourceBuff);
		}
	}
}