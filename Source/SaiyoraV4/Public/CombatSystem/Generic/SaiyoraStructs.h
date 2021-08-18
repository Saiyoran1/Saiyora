// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

DECLARE_DELEGATE(FModifierCallback);
DECLARE_MULTICAST_DELEGATE(FModifierNotification);
DECLARE_DYNAMIC_DELEGATE(FGenericCallback);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenericNotification);

USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    FCombatModifier();
    FCombatModifier(float const Value, EModifierType const ModType, bool const bStackable = false, class UBuff* Source = nullptr);
    float GetValue() const { return ModValue; }
    EModifierType GetModType() const { return ModType; }
    bool GetStackable() const { return bStackable; }
    UBuff* GetSource() const { return Source; }
private:
    UPROPERTY()
    UBuff* Source = nullptr;
    EModifierType ModType = EModifierType::Invalid;
    float ModValue = 0.0f;
    bool bStackable = true;
    static int32 GlobalID;
public:
    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static int32 ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue);
    static int32 GetID();
};

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(float, FFloatValueRecalculation, TArray<FCombatModifier>const&, Modifiers, float const, BaseValue);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FFloatValueCallback, float const, Previous, float const, New);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFloatValueNotification, float const, Previous, float const, New);

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(int32, FIntValueRecalculation, TArray<FCombatModifier>const&, Modifiers, int32 const, BaseValue);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FIntValueCallback, int32 const, Previous, int32 const, New);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIntValueNotification, int32 const, Previous, int32 const, New);

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
    TSubclassOf<UObject> Class;
    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadWrite)
    FVector Scale = FVector::ZeroVector;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        SerializeOptionalValue(Ar.IsSaving(), Ar, ParamType, ECombatParamType::None);
        SerializeOptionalValue(Ar.IsSaving(), Ar, ID, 0);
        SerializeOptionalValue(Ar.IsSaving(), Ar, Object, static_cast<UObject*>(nullptr));
        SerializeOptionalValue(Ar.IsSaving(), Ar, Class, static_cast<TSubclassOf<UObject>>(nullptr));
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

    UPROPERTY(BlueprintReadWrite, Category = "Abilities")
    TArray<FCombatParameter> Parameters;

    bool HasParams() const { return Parameters.Num() > 0; }
    void ClearParams() { Parameters.Empty(); }
};