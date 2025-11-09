#include "CrosshairManager.h"

#include "AbilityComponent.h"
#include "CombatStatusComponent.h"
#include "FireWeapon.h"
#include "ModernCrosshair.h"
#include "SaiyoraPlayerCharacter.h"

void UCrosshairManager::Init(ASaiyoraPlayerCharacter* PlayerRef)
{
	if (!IsValid(PlayerRef))
	{
		return;
	}
	PlayerChar = PlayerRef;
	UCombatStatusComponent* OwnerCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(PlayerChar);
	if (IsValid(OwnerCombatStatusComp))
	{
		OwnerCombatStatusComp->OnPlaneSwapped.AddDynamic(this, &UCrosshairManager::OnPlaneSwap);
		OnPlaneSwap(ESaiyoraPlane::Neither, OwnerCombatStatusComp->GetCurrentPlane(), nullptr);
	}
	UAbilityComponent* OwnerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(PlayerChar);
	if (IsValid(OwnerAbilityComp))
	{
		OwnerAbilityComp->OnAbilityAdded.AddDynamic(this, &UCrosshairManager::OnAbilityAdded);
		OwnerAbilityComp->OnAbilityRemoved.AddDynamic(this, &UCrosshairManager::OnAbilityRemoved);
		UFireWeapon* WeaponAbility = Cast<UFireWeapon>(OwnerAbilityComp->FindActiveAbility(UFireWeapon::StaticClass()));
		if (IsValid(WeaponAbility))
		{
			OnAbilityAdded(WeaponAbility);
		}
	}
}

void UCrosshairManager::UpdateCrosshairVisibility(const ESaiyoraPlane NewPlane)
{
	//If we have no modern crosshair, we'll pretend we're in Ancient Plane at all times.
	const ESaiyoraPlane ActualPlane = IsValid(ModernCrosshair) ? NewPlane : ESaiyoraPlane::Ancient;
	CachedLastPlane = ActualPlane;
	if (ActualPlane == ESaiyoraPlane::Ancient)
	{
		AncientCrosshair->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (IsValid(ModernCrosshair))
		{
			ModernCrosshair->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		AncientCrosshair->SetVisibility(ESlateVisibility::Collapsed);
		if (IsValid(ModernCrosshair))
		{
			ModernCrosshair->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UCrosshairManager::OnAbilityAdded(UCombatAbility* NewAbility)
{
	if (!IsValid(NewAbility))
	{
		return;
	}
	//Check if the new ability is a weapon ability, so we can decide whether to update our crosshair.
	UFireWeapon* WeaponAbility = Cast<UFireWeapon>(NewAbility);
	if (!IsValid(WeaponAbility) || WeaponAbility == CurrentWeaponAbility)
	{
		return;
	}
	CurrentWeaponAbility = WeaponAbility;
	//Cleanup the old modern crosshair, if we had one.
	if (IsValid(ModernCrosshair))
	{
		ModernCrosshair->RemoveFromParent();
		ModernCrosshair = nullptr;
	}
	//If the new weapon has a crosshair class, use it.
	if (IsValid(WeaponAbility->GetCrosshairClass()))
	{
		ModernCrosshair = CreateWidget<UModernCrosshair>(this, WeaponAbility->GetCrosshairClass());
		if (IsValid(ModernCrosshair))
		{
			CrosshairCanvas->AddChildToCanvas(ModernCrosshair);
			ModernCrosshair->Init(PlayerChar, WeaponAbility);
		}
	}
	//Update to show the appropriate crosshair on screen if it changed.
	UpdateCrosshairVisibility(CachedLastPlane);
}

void UCrosshairManager::OnAbilityRemoved(UCombatAbility* RemovedAbility)
{
	if (RemovedAbility == CurrentWeaponAbility)
	{
		ModernCrosshair->RemoveFromParent();
		ModernCrosshair = nullptr;
		//Update to show the appropriate crosshair on screen if it changed.
		UpdateCrosshairVisibility(CachedLastPlane);
	}
}