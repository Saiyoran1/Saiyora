// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

DECLARE_DELEGATE(FModifierCallback);
DECLARE_MULTICAST_DELEGATE(FModifierNotification);

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

    void Reset();
    void Activate();
    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static void CombineModifiers(TArray<FCombatModifier> const& ModArray, FCombatModifier& OutAddMod, FCombatModifier& OutMultMod);

    FModifierNotification OnStacksChanged;
    FModifierNotification OnSourceRemoved;
    void OnBuffStacked(struct FBuffApplyEvent const& Event);
    void OnBuffRemoved(struct FBuffRemoveEvent const& Event);
};

USTRUCT()
struct FModifierCollection
{
    GENERATED_BODY()
private:
    static int32 GlobalID;
    TMap<int32, FCombatModifier> IndividualModifiers;
    FModifierNotification OnModifiersChanged;
    FCombatModifier SummedAddMod;
    FCombatModifier SummedMultMod;
    void RecalculateMods();
    void PurgeInvalidMods();
public:
    static int32 GetID();
    int32 AddModifier(FCombatModifier const& Modifier);
    void RemoveModifier(int32 const ModifierID);
    void GetSummedModifiers(TArray<FCombatModifier>& OutMods) const { OutMods.Add(SummedAddMod); OutMods.Add(SummedMultMod); }
    void GetIndividualModifiers(TArray<FCombatModifier>& OutMods) const { IndividualModifiers.GenerateValueArray(OutMods); }
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

DECLARE_DELEGATE_RetVal_OneParam(float, FCombatValueRecalculation, TArray<FCombatModifier>const&)

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

    void AddDependency(FModifierCollection* NewDependency);
    void RemoveDependency(FModifierCollection* NewDependency);
    void GetDependencyModifiers(TArray<FCombatModifier>& OutMods);
    void SetRecalculationFunction(FCombatValueRecalculation const& NewCalculation);
private:
    void RecalculateValue();
    TArray<FModifierCollection*> Dependencies;
    void DefaultRecalculation();
    FCombatValueRecalculation CustomCalculation;
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