// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

USTRUCT()
struct FCombatFloatValue
{
    GENERATED_BODY()

    float BaseValue = 0.0f;
    bool bModifiable = false;
    bool bCappedLow = false;
    float Minimum = 0.0f;
    bool bCappedHigh = false;
    float Maximum = 0.0f;
    float Value = 0.0f;
};

USTRUCT()
struct FCombatIntValue
{
    GENERATED_BODY()

    int32 BaseValue = 0;
    bool bModifiable = false;
    bool bCappedLow = false;
    int32 Minimum = 0;
    bool bCappedHigh = false;
    int32 Maximum = 0;
    int32 Value = 0;
};

USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    UPROPERTY()
    class UBuff* Source = nullptr;
    bool bFromBuff = false;
    UPROPERTY(BlueprintReadOnly)
    EModifierType ModType = EModifierType::Invalid;
    UPROPERTY(BlueprintReadOnly)
    float ModValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    bool bStackable = true;

    static float CombineModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static FCombatModifier CombineAdditiveMods(TArray<FCombatModifier> const& Mods);
    static FCombatModifier CombineMultiplicativeMods(TArray<FCombatModifier> const& Mods);

    static int32 GlobalID;
    static int32 GetID();
};

DECLARE_DELEGATE_TwoParams(FModifierCallback, FCombatModifier const&, FCombatModifier const&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FModifierNotification, FCombatModifier const&, FCombatModifier const&);

USTRUCT()
struct FModifierCollection
{
    GENERATED_BODY()
private:
    TMap<int32, FCombatModifier> Modifiers;
    FModifierNotification OnModifiersChanged;
public:
    int32 AddModifier(FCombatModifier const& Modifier);
    void RemoveModifier(int32 const ModifierID);
    void GetModifiers(TArray<FCombatModifier>& OutMods);
    FDelegateHandle BindToModsChanged(FModifierCallback const& Callback)
    {
        if (Callback.IsBound())
        {
            return OnModifiersChanged.Add(Callback);
        }
        FDelegateHandle const Invalid;
        return Invalid;
    }
    void UnbindFromModsChanged(FDelegateHandle const& Handle)
    {
        if (Handle.IsValid())
        {
            OnModifiersChanged.Remove(Handle);
        }
    }
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