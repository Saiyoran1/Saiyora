// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "PlayerAbilityHandler.h"
#include "UObject/NoExportTypes.h"
#include "PlayerSpecialization.generated.h"

struct FAbilityTalentInfo;
class UCombatAbility;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UPlayerSpecialization : public UObject
{
	GENERATED_BODY()
};