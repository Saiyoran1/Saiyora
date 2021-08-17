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
    ~FCombatModifier();
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
    void OnBuffStacked();
    void OnBuffRemoved();
    FModifierNotification OnModifierChanged;
    FDelegateHandle StackHandle;
    FDelegateHandle RemoveHandle;
    static int32 GlobalID;
public:
    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static int32 ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue);
    static void CombineModifiers(TArray<FCombatModifier> const& ModArray, FCombatModifier& OutAddMod, FCombatModifier& OutMultMod);
    static int32 GetID();
};

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(float, FFloatValueRecalculation, TArray<FCombatModifier>const&, Modifiers, float const, BaseValue);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FFloatValueCallback, float const, Previous, float const, New);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFloatValueNotification, float const, Previous, float const, New);

USTRUCT()
struct FCombatFloatValue
{
    GENERATED_BODY()

    FCombatFloatValue();
    FCombatFloatValue(float const BaseValue, bool const bModifiable = false, bool const bHasMin = false, float const Min = 0.0f, bool const bHasMax = false, float const Max = 0.0f);
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

DECLARE_DELEGATE_RetVal_TwoParams(int32, FCombatIntValueRecalculation, TArray<FCombatModifier>const&, int32 const);
DECLARE_DELEGATE_TwoParams(FIntValueCallback, int32 const, int32 const);
DECLARE_MULTICAST_DELEGATE_TwoParams(FIntValueNotification, int32 const, int32 const);

USTRUCT()
struct FCombatIntValue
{
    GENERATED_BODY()

    FCombatIntValue();
    FCombatIntValue(int32 const BaseValue, bool const bModifiable = false, bool const bHasMin = false, int32 const Min = 0, bool const bHasMax = false, int32 const Max = 0);
    int32 GetValue() const { return Value; }
    bool IsModifiable() const { return bModifiable; }
    void SetRecalculationFunction(FCombatIntValueRecalculation const& NewCalculation);
    int32 ForceRecalculation();
    FDelegateHandle BindToValueChanged(FIntValueCallback const& Callback);
    void UnbindFromValueChanged(FDelegateHandle const& Handle);
    int32 AddModifier(FCombatModifier const& Modifier);
    void RemoveModifier(int32 const ModifierID);
private:
    bool bModifiable = false;
    bool bHasMin = false;
    int32 Minimum = 0;
    bool bHasMax = false;
    int32 Maximum = 0;
    int32 BaseValue = 0;
    int32 Value = 0;
    FModifierCollection Modifiers;
    void RecalculateValue();
    void DefaultRecalculation();
    FCombatIntValueRecalculation CustomCalculation;
    FIntValueNotification OnValueChanged;
};

UCLASS()
class UModifiableFloatValue : public UObject
{
    GENERATED_BODY()
public:
    void Init(float const BaseValue, bool const bModifiable = false, bool const bHasMin = false, float const Minimum = 0.0f, bool const bHasMax, float const Maximum = 0.0f);
    void SetRecalculationFunction(FFloatValueRecalculation const& NewCalculation);
    void RecalculateValue();
    void SubscribeToValueChanged(FFloatValueCallback const& Callback);
    void UnsubscribeFromValueChanged(FFloatValueCallback const& Callback);
    int32 AddModifier(FCombatModifier const& Modifier);
    void RemoveModifier(int32 const ModifierID);
    float GetValue() const { return Value; }
    bool IsModifiable() const { return bModifiable; }
private:
    float Value = 0.0f;
    float BaseValue = 0.0f;
    bool bModifiable = false;
    bool bHasMin = false;
    float Minimum = 0.0f;
    bool bHasMax = false;
    float Maximum = 0.0f;
    TMap<int32, FCombatModifier> Modifiers;
    FFloatValueRecalculation CustomRecalculation;
    FFloatValueNotification OnValueChanged;
    struct FBuffEventCallback BuffStackCallback;
    UFUNCTION()
    void CheckForModifierSourceStack(struct FBuffApplyEvent const& Event);
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