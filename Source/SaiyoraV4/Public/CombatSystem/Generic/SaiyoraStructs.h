// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
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

USTRUCT(BlueprintType)
struct FCombatFloatParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, meta = (Categories = "Param"))
    FGameplayTag ParamType;
    UPROPERTY(BlueprintReadWrite)
    float Value;

    bool IsValidParam() const { return ParamType.IsValid(); }
};

USTRUCT(BlueprintType)
struct FCombatPointerParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, meta = (Categories = "Param"))
    FGameplayTag ParamType;
    UPROPERTY(BlueprintReadWrite)
    UObject* Value;

    bool IsValidParam() const { return ParamType.IsValid() && IsValid(Value); }
};

USTRUCT(BlueprintType)
struct FCombatTagParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, meta = (Categories = "Param"))
    FGameplayTag ParamType;
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag Value;

    bool IsValidParam() const { return ParamType.IsValid() && Value.IsValid(); }
};

USTRUCT(BlueprintType)
struct FCombatParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FCombatFloatParam> FloatParams;
    UPROPERTY(BlueprintReadWrite)
    TArray<FCombatPointerParam> PointerParams;
    UPROPERTY(BlueprintReadWrite)
    TArray<FCombatTagParam> TagParams;

    bool HasParams() const { return FloatParams.Num() != 0 || PointerParams.Num() != 0 || TagParams.Num() != 0; }
};