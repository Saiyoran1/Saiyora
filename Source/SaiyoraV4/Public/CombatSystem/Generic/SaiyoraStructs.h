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
struct FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    ECombatParamType ParamType = ECombatParamType::None;
    UPROPERTY(BlueprintReadWrite)
    int32 ID = 0;
    UPROPERTY(BlueprintReadWrite)
    UObject* Object = nullptr;
    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadWrite)
    FVector Scale = FVector::ZeroVector;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << ParamType;
        Ar << ID;
        Ar << Object;
        NetSerializeOptionalValue(Ar.IsSaving(), Ar, Location, FVector::ZeroVector, Map);
        NetSerializeOptionalValue(Ar.IsSaving(), Ar, Rotation, FRotator::ZeroRotator, Map);
        NetSerializeOptionalValue(Ar.IsSaving(), Ar, Scale, FVector::ZeroVector, Map);
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FCombatParameter> : public TStructOpsTypeTraitsBase2<FCombatParameter>
{
    enum
    {
        WithNetSerializer = true
    };
};

USTRUCT(BlueprintType)
struct FCombatParameters
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FCombatParameter> Parameters;

    bool HasParams() const { return Parameters.Num() > 0; }
};