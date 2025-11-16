// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Image.h"
#include "UserWidget.h"
#include "ModernCrosshair.generated.h"

class UFireWeapon;
class ASaiyoraPlayerCharacter;

UCLASS(Abstract)
class SAIYORAV4_API UModernCrosshair : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* Player, UFireWeapon* OwningWeapon);

protected:

	virtual void UpdateSpread(const float SpreadAngle, const float OptimalDisplayRange, UMaterialInstanceDynamic* DynamicMat, UImage* CrosshairImage) {}
	UFireWeapon* GetWeaponRef() const { return WeaponRef; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UMaterialInterface* CrosshairMat;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMI = nullptr;
	UPROPERTY(meta = (BindWidget))
	UImage* Img_Crosshair;
	UFUNCTION()
	void OnSpreadUpdated(const float NewSpread);

	UPROPERTY()
	UFireWeapon* WeaponRef = nullptr;
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerRef = nullptr;
};
