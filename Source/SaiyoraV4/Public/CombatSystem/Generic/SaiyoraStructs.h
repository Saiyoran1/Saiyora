#pragma once
#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static int32 ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue);
    FCombatModifier();
    FCombatModifier(float const BaseValue, EModifierType const ModifierType, class UBuff* SourceBuff = nullptr, bool const Stackable = false);
    UPROPERTY()
    EModifierType Type = EModifierType::Invalid;
    UPROPERTY()
    float Value = 0.0f;
    UPROPERTY()
    bool bStackable = true;
    UPROPERTY(NotReplicated)
    UBuff* Source = nullptr;
    UPROPERTY()
    int32 Stacks = 0;

    FORCEINLINE bool operator==(FCombatModifier const& Other) const { return Other.Type == Type && Other.Source == Source && Other.Value == Value; }
};

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