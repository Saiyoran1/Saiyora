// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilityEnums.h"
#include "SaiyoraStructs.h"
#include "AbilityStructs.generated.h"

USTRUCT()
struct FReplicatedAbilityCost : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FORCEINLINE bool operator==(const FReplicatedAbilityCost& Other) const { return Other.ResourceTag.MatchesTag(ResourceTag); }

    void PostReplicatedChange(struct FAbilityCostArray const& InArraySerializer);
    void PostReplicatedAdd(struct FAbilityCostArray const& InArraySerializer);
    void PreReplicatedRemove(struct FAbilityCostArray const& InArraySerializer);

    UPROPERTY(meta = (Categories = "Resource"))
    FGameplayTag ResourceTag;
    UPROPERTY()
    float Cost = 0.0f;
    UPROPERTY()
    bool bStaticCost = false;
};

USTRUCT()
struct FAbilityCostArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FReplicatedAbilityCost> Items;
    UPROPERTY(NotReplicated)
    class UCombatAbility* Ability;
    
    bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedAbilityCost, FAbilityCostArray>( Items, DeltaParms, *this );
    }
};

USTRUCT(BlueprintType)
struct FAbilityCost
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Resource"))
    FGameplayTag ResourceTag;
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
struct FAbilityFloatParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    EFloatParamType ParamType = EFloatParamType::None;
    UPROPERTY(BlueprintReadWrite)
    float ParamValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FAbilityPointerParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    EPointerParamType ParamType = EPointerParamType::None;
    UPROPERTY(BlueprintReadWrite)
    AActor* ParamPointer = nullptr;
};

USTRUCT(BlueprintType)
struct FAbilityTagParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    ETagParamType ParamType = ETagParamType::None;
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag ParamTag;
};

USTRUCT(BlueprintType)
struct FAbilityRepParams
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FAbilityFloatParam> FloatParams;
    UPROPERTY(BlueprintReadWrite)
    TArray<FAbilityPointerParam> PointerParams;
    UPROPERTY(BlueprintReadWrite)
    TArray<FAbilityTagParam> TagParams;
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
    FString FailReason;
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

USTRUCT(BlueprintType)
struct FInterruptEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    FString FailReason;
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
    TSubclassOf<UCombatAbility> AbilityClass;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    int32 Tick = 0;
    UPROPERTY()
    float ClientStartTime = 0.0f;
    UPROPERTY()
    FAbilityRepParams PredictionParams;
};

USTRUCT()
struct FClientAbilityPrediction
{
    GENERATED_BODY()
    
    UPROPERTY()
    UCombatAbility* Ability = nullptr;
    int32 PredictionID = 0;
    float ClientTime = 0.0f;
    bool bPredictedGCD = false;
    bool bPredictedCharges = false;
    bool bPredictedCastBar = false;
    FGameplayTagContainer PredictedCostTags;
};

USTRUCT()
struct FServerAbilityResult
{
    GENERATED_BODY()

    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    TSubclassOf<UCombatAbility> AbilityClass;
    UPROPERTY()
    float ClientStartTime = 0.0f;
    UPROPERTY()
    bool bActivatedGlobal = false;
    UPROPERTY()
    float GlobalLength = 0.0f;
    UPROPERTY()
    bool bSpentCharges = false;
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
    return FCrc::MemCrc32(&Tick, sizeof(FPredictedTick));
}

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FAbilityModCondition, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FAbilityRestriction, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityInstanceCallback, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCallback, FCastEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCancelCallback, FCancelEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FInterruptCallback, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAbilityChargeCallback, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGlobalCooldownCallback, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCastingStateCallback, FCastingState const&, OldState, FCastingState const&, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityNotification, FCastEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityCancelNotification, FCancelEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterruptNotification, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityChargeNotification, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInstanceNotification, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGlobalCooldownNotification, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCastingStateNotification, FCastingState const&, OldState, FCastingState const&, NewState);