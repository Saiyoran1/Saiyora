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
    FString ParamName;
    UPROPERTY(BlueprintReadWrite)
    bool BoolParam = false;
    UPROPERTY(BlueprintReadWrite)
    int32 IntParam = 0;
    UPROPERTY(BlueprintReadWrite)
    float FloatParam = 0.0f;
    UPROPERTY(BlueprintReadWrite)
    UObject* ObjectParam = nullptr;
    UPROPERTY(BlueprintReadWrite)
    TSubclassOf<UObject> ClassParam;
    UPROPERTY(BlueprintReadWrite)
    FVector VectorParam = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FRotator RotatorParam = FRotator::ZeroRotator;

    friend FArchive& operator<<(FArchive& Ar, FCombatParameter& Parameter)
    {
        Ar << Parameter.ParamType;
        Ar << Parameter.ParamName;
        switch (Parameter.ParamType)
        {
            case ECombatParamType::None :
                break;
            case ECombatParamType::String :
                break;
            case ECombatParamType::Bool :
                Ar << Parameter.BoolParam;
                break;
            case ECombatParamType::Int :
                Ar << Parameter.IntParam;
                break;
            case ECombatParamType::Float :
                Ar << Parameter.FloatParam;
                break;
            case ECombatParamType::Object :
                Ar << Parameter.ObjectParam;
                break;
            case ECombatParamType::Class :
                Ar << Parameter.ClassParam;
                break;
            case ECombatParamType::Vector :
                Ar << Parameter.VectorParam;
                break;
            case ECombatParamType::Rotator :
                Ar << Parameter.RotatorParam;
                break;
            default :
                break;
        }
        return Ar;
    }
};

USTRUCT(BlueprintType)
struct FCombatParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Abilities")
    TArray<FCombatParameter> Parameters;

    bool HasParams() const { return Parameters.Num() > 0; }
    void ClearParams() { Parameters.Empty(); }

    friend FArchive& operator<<(FArchive& Ar, FCombatParameters& Parameters)
    {
        Ar << Parameters.Parameters;
        return Ar;
    }
};