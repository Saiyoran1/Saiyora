// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FCombatModifier
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite)
  EModifierType ModifierType = EModifierType::Invalid;
  UPROPERTY(BlueprintReadWrite)
  float ModifierValue = 0.0f;
};