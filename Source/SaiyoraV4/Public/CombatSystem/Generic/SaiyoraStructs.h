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

  UPROPERTY(BlueprintReadWrite, Category = "Modifier")
  EModifierType ModifierType = EModifierType::Invalid;
  UPROPERTY(BlueprintReadWrite, Category = "Modifier", meta = (ClampMin = "0.0"))
  float ModifierValue = 0.0f;
  UPROPERTY(BlueprintReadWrite, Category = "Modifier", meta = (ClampMin = "1.0"))
  int32 Stacks = 1;

  static float CombineModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
};