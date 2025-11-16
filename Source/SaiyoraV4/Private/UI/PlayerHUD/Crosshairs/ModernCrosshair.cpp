// Fill out your copyright notice in the Description page of Project Settings.

#include "ModernCrosshair.h"
#include "FireWeapon.h"
#include "Image.h"
#include "SaiyoraPlayerCharacter.h"

void UModernCrosshair::Init(ASaiyoraPlayerCharacter* Player, UFireWeapon* OwningWeapon)
{
	if (!IsValid(Player) || !IsValid(OwningWeapon))
	{
		return;
	}
	PlayerRef = Player;
	WeaponRef = OwningWeapon;
	if (IsValid(CrosshairMat))
	{
		DynamicMI = UMaterialInstanceDynamic::Create(CrosshairMat, this);
		if (IsValid(DynamicMI))
		{
			Img_Crosshair->SetBrushFromMaterial(DynamicMI);
		}
	}
	if (WeaponRef->UsesSpread())
	{
		WeaponRef->OnSpreadChanged.AddDynamic(this, &UModernCrosshair::OnSpreadUpdated);
        OnSpreadUpdated(WeaponRef->GetCurrentSpreadAngle());
	}
}

void UModernCrosshair::OnSpreadUpdated(const float NewSpread)
{
	UpdateSpread(NewSpread, WeaponRef->GetOptimalDisplayRange(), DynamicMI, Img_Crosshair);
}