// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UserWidget.h"
#include "ModernCrosshair.generated.h"

class UFireWeapon;
class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API UModernCrosshair : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* Player, UFireWeapon* OwningWeapon);
	
};
