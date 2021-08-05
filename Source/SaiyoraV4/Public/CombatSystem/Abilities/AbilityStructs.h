// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "Resource.h"
#include "AbilityStructs.generated.h"

class UPlayerCombatAbility;

USTRUCT(BlueprintType)
struct FAbilityCost
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<UResource> ResourceClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Cost = 0.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bStaticCost = false;
};

USTRUCT()
struct FAbilityCooldown
{
    GENERATED_BODY()

    UPROPERTY()
    int32 CurrentCharges = 0;
    UPROPERTY()
    bool OnCooldown = false;
    UPROPERTY()
    float CooldownStartTime = 0.0f;
    UPROPERTY()
    float CooldownEndTime = 0.0f;
    UPROPERTY()
    int32 PredictionID = 0;
};

USTRUCT(BlueprintType)
struct FCastEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECastAction ActionTaken = ECastAction::Fail;
    UPROPERTY(BlueprintReadOnly)
    class UCombatAbility* Ability = nullptr;
    UPROPERTY(BlueprintReadOnly)
    int32 Tick = 0;
    UPROPERTY(BlueprintReadOnly)
    ECastFailReason FailReason = ECastFailReason::None;
    UPROPERTY()
    int32 PredictionID = 0;
};

USTRUCT(BlueprintType)
struct FCancelEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    ECancelFailReason FailReason = ECancelFailReason::None;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* CancelledAbility = nullptr;
    UPROPERTY()
    int32 CancelID = 0;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY(BlueprintReadOnly)
    float CancelledCastStart = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CancelledCastEnd = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CancelTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    int32 ElapsedTicks = 0;
};

USTRUCT()
struct FCancelRequest
{
    GENERATED_BODY()

    UPROPERTY()
    int32 CancelID = 0;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY()
    float CancelTime = 0.0f;
    UPROPERTY()
    FCombatParameters PredictionParams;
};

USTRUCT(BlueprintType)
struct FInterruptEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    EInterruptFailReason FailReason = EInterruptFailReason::None;
    UPROPERTY(BlueprintReadOnly)
    AActor* InterruptAppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly)
    AActor* InterruptAppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* InterruptedAbility = nullptr;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY(BlueprintReadOnly)
    UObject* InterruptSource = nullptr;
    UPROPERTY(BlueprintReadOnly)
    float InterruptedCastStart = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float InterruptedCastEnd = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float InterruptTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    int32 ElapsedTicks = 0;
};

USTRUCT(BlueprintType)
struct FGlobalCooldown
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bGlobalCooldownActive = false;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float StartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FCastingState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsCasting = false;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* CurrentCast = nullptr;
    UPROPERTY(NotReplicated)
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float CastStartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CastEndTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    bool bInterruptible = true;
    UPROPERTY(BlueprintReadOnly, NotReplicated)
    int32 ElapsedTicks = 0;
};

USTRUCT()
struct FAbilityRequest
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<UPlayerCombatAbility> AbilityClass;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    int32 Tick = 0;
    UPROPERTY()
    float ClientStartTime = 0.0f;
    UPROPERTY()
    FCombatParameters PredictionParams;
};

USTRUCT()
struct FClientAbilityPrediction
{
    GENERATED_BODY()
    
    UPROPERTY()
    UPlayerCombatAbility* Ability = nullptr;
    int32 PredictionID = 0;
    float ClientTime = 0.0f;
    bool bPredictedGCD = false;
    bool bPredictedCharges = false;
    bool bPredictedCastBar = false;
    UPROPERTY()
    TArray<TSubclassOf<UResource>> PredictedCostClasses;
};

USTRUCT()
struct FServerAbilityResult
{
    GENERATED_BODY()

    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    TSubclassOf<UPlayerCombatAbility> AbilityClass;
    UPROPERTY()
    float ClientStartTime = 0.0f;
    UPROPERTY()
    bool bActivatedGlobal = false;
    UPROPERTY()
    float GlobalLength = 0.0f;
    UPROPERTY()
    int32 ChargesSpent = 0;
    UPROPERTY()
    bool bActivatedCastBar = false;
    UPROPERTY()
    float CastLength = 0.0f;
    UPROPERTY()
    bool bInterruptible = false;
    UPROPERTY()
    TArray<FAbilityCost> AbilityCosts;
};

USTRUCT()
struct FPredictedTick
{
    GENERATED_BODY()

    int32 PredictionID;
    int32 TickNumber;

    FORCEINLINE bool operator==(const FPredictedTick& Other) const { return Other.PredictionID == PredictionID && Other.TickNumber == TickNumber; }
};

FORCEINLINE uint32 GetTypeHash(const FPredictedTick& Tick)
{
    return HashCombine(GetTypeHash(Tick.PredictionID), GetTypeHash(Tick.TickNumber));
}

USTRUCT()
struct FAbilityModCollection
{
    GENERATED_BODY()
private:
    bool bInitialized = false;
//Class Specific Modifiers
    FModifierCollection MaxChargeModifiers;
    FModifierCollection* GenericMaxChargeModifiers = nullptr;
    FDelegateHandle MaxChargeHandle;
    void RecalculateMaxCharges();
    FModifierCollection ChargesPerCastModifiers;
    FModifierCollection* GenericChargesPerCastModifiers = nullptr;
    FDelegateHandle ChargesPerCastHandle;
    void RecalculateChargesPerCast();
    FModifierCollection ChargesPerCooldownModifiers;
    FModifierCollection* GenericChargesPerCooldownModifiers = nullptr;
    FDelegateHandle ChargesPerCooldownHandle;
    void RecalculateChargesPerCooldown();
    FModifierCollection GlobalCooldownModifiers;
    FDelegateHandle GlobalCooldownHandle;
    void RecalculateGlobalCooldownLength();
    FModifierCollection CooldownModifiers;
    FModifierCollection* GenericCooldownModifiers = nullptr;
    FDelegateHandle CooldownHandle;
    void RecalculateCooldownLength();
    FModifierCollection CastLengthModifiers;
    FModifierCollection* GenericCastLengthModifiers = nullptr;
    FDelegateHandle CastLengthHandle;
    void RecalculateCastLength();
    TMap<TSubclassOf<UResource>, FModifierCollection> CostModifiers;
//Calculated Values
    FCombatIntValue MaxCharges;
    FCombatIntValue ChargesPerCast;
    FCombatIntValue ChargesPerCooldown;
    FCombatFloatValue GlobalCooldownLength;
    FCombatFloatValue CooldownLength;
    FCombatFloatValue CastLength;
    TMap<TSubclassOf<UResource>, FCombatFloatValue> AbilityCosts;
public:
    void Initialize(TSubclassOf<UCombatAbility> const AbilityClass, FModifierCollection& GenericMaxChargeMods,
        FModifierCollection& GenericChargesPerCastMods, FModifierCollection& GenericChargesPerCooldownMods,
        FModifierCollection& GenericGlobalCooldownLengthMods, FModifierCollection& GenericCooldownLengthMods,
        FModifierCollection& GenericCastLengthMods, TMap<TSubclassOf<UResource>, FModifierCollection&> const& GenericCostMods);

    int32 AddMaxChargeModifier(FCombatModifier const& Modifier);
    void RemoveMaxChargeModifier(int32 const ModifierID);
    int32 AddChargesPerCastModifier(FCombatModifier const& Modifier);
    void RemoveChargesPerCastModifier(int32 const ModifierID);
    int32 AddChargesPerCooldownModifier(FCombatModifier const& Modifier);
    void RemoveChargesPerCooldownModifier(int32 const ModifierID);
    int32 AddGlobalCooldownModifier(FCombatModifier const& Modifier);
    void RemoveGlobalCoolodownModifier(int32 const ModifierID);
    int32 AddCooldownModifier(FCombatModifier const& Modifier);
    void RemoveCooldownModifier(int32 const ModifierID);
    int32 AddCastLengthModifier(FCombatModifier const& Modifier);
    void RemoveCastLengthModifier(int32 const ModifierID);
    int32 AddCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
    void RemoveCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID);

    int32 GetMaxCharges() const { return MaxCharges.Value; }
    int32 GetChargesPerCast() const { return ChargesPerCast.Value; }
    int32 GetChargesPerCooldown() const { return ChargesPerCooldown.Value; }
    float GetGlobalCooldownLength() const { return GlobalCooldownLength.GetValue(); }
    float GetCooldownLength() const { return CooldownLength.GetValue(); }
    float GetCastLength() const { return CastLength.GetValue(); }
    float GetCost(TSubclassOf<UResource> const ResourceClass) const { return AbilityCosts.FindRef(ResourceClass).GetValue(); }
    void GetCosts(TMap<TSubclassOf<UResource>, float>& OutCosts);
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FAbilityModCondition, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FAbilityRestriction, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FInterruptRestriction, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityInstanceCallback, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCallback, FCastEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCancelCallback, FCancelEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FInterruptCallback, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAbilityChargeCallback, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGlobalCooldownCallback, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCastingStateCallback, FCastingState const&, OldState, FCastingState const&, NewState);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FAbilityMispredictionCallback, int32 const, PredictionID, ECastFailReason const, FailReason);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAbilityBindingCallback, int32 const, AbilityBind, EActionBarType const, Bar, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBarSwapCallback, ESaiyoraPlane const, NewPlane);
//Spellbook delegate has no params because TSubclassOf in dynamic delegate causes compile errors :(
DECLARE_DYNAMIC_DELEGATE(FSpellbookCallback);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSpecializationCallback, class UPlayerSpecialization*, NewSpecialization);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityNotification, FCastEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityCancelNotification, FCancelEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterruptNotification, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityChargeNotification, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInstanceNotification, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGlobalCooldownNotification, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCastingStateNotification, FCastingState const&, OldState, FCastingState const&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityMispredictionNotification, int32 const, PredictionID, ECastFailReason const, FailReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityBindingNotification, int32 const, AbilityBind, EActionBarType const, Bar, UCombatAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBarSwapNotification, EActionBarType const, NewBar);
//Spellbook delegate has no params because TSubclassOf in dynamic delegate causes compile errors :(
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpellbookNotification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecializationNotification, class UPlayerSpecialization*, NewSpecialization);