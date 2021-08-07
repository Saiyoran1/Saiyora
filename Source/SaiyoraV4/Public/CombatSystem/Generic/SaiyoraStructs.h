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
    
    void Reset();
    void Activate(FModifierCallback const& OnChangedCallback);
    void SetValue(float const NewValue);
    void SetModType(EModifierType const NewModType);
    void SetStackable(bool const NewStackable);
    void SetSource(class UBuff* NewSource);
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
    void OnBuffStacked(struct FBuffApplyEvent const& Event);
    void OnBuffRemoved(struct FBuffRemoveEvent const& Event);
    FModifierNotification OnModifierChanged;
public:
    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static void CombineModifiers(TArray<FCombatModifier> const& ModArray, FCombatModifier& OutAddMod, FCombatModifier& OutMultMod);
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
    FDelegateHandle BindToModsChanged(FModifierCallback const& Callback);
    void UnbindFromModsChanged(FDelegateHandle const& Handle);
};

DECLARE_DELEGATE_RetVal_TwoParams(float, FCombatValueRecalculation, TArray<FCombatModifier>const&, float const);
DECLARE_DELEGATE_TwoParams(FFloatValueCallback, float const, float const);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFloatValueNotification, float const, float const);

USTRUCT()
struct FCombatFloatValue
{
    GENERATED_BODY()

    FCombatFloatValue();
    FCombatFloatValue(float const BaseValue, bool const bModifiable = false, bool const HasMin = false, float const Min = 0.0f, bool const bHasMax = false, float const Max = 0.0f);
    float GetValue() const { return Value; }
    bool IsModifiable() const { return bModifiable; }
    void SetRecalculationFunction(FCombatValueRecalculation const& NewCalculation);
    float ForceRecalculation();
    FDelegateHandle BindToValueChanged(FFloatValueCallback const& Callback);
    void UnbindFromValueChanged(FDelegateHandle const& Handle);
    int32 AddModifier(FCombatModifier const& Modifier);
    void RemoveModifier(int32 const ModifierID);
private:
    bool bModifiable = false;
    bool bHasMin = false;
    float Minimum = 0.0f;
    bool bHasMax = false;
    float Maximum = 0.0f;
    float BaseValue = 0.0f;
    float Value = 0.0f;
    FModifierCollection Modifiers;
    void RecalculateValue();
    void DefaultRecalculation();
    FCombatValueRecalculation CustomCalculation;
    FFloatValueNotification OnValueChanged;
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