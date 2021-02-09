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
    int32 LastAckedID = 0;
};

USTRUCT(BlueprintType)
struct FAbilityFloatParam
{
    GENERATED_BODY()

    UPROPERTY()
    EFloatParamType ParamType = EFloatParamType::None;
    UPROPERTY()
    float ParamValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FAbilityPointerParam
{
    GENERATED_BODY()

    UPROPERTY()
    EPointerParamType ParamType = EPointerParamType::None;
    UPROPERTY()
    AActor* ParamPointer = nullptr;
};

USTRUCT(BlueprintType)
struct FAbilityTagParam
{
    GENERATED_BODY()

    UPROPERTY()
    ETagParamType ParamType = ETagParamType::None;
    UPROPERTY(meta = (Categories = "Param"))
    FGameplayTag ParamTag;
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
    int32 CastID = 0;
};

USTRUCT(BlueprintType)
struct FCancelEvent
{
    GENERATED_BODY()

    UPROPERTY()
    bool bSuccess = false;
    UPROPERTY()
    UCombatAbility* CancelledAbility = nullptr;
    UPROPERTY()
    int32 CancelID = 0;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY()
    float CancelledCastStart = 0.0f;
    UPROPERTY()
    float CancelledCastEnd = 0.0f;
    UPROPERTY()
    float CancelTime = 0.0f;
    UPROPERTY()
    int32 ElapsedTicks = 0;
};

USTRUCT(BlueprintType)
struct FInterruptEvent
{
    GENERATED_BODY()

    UPROPERTY()
    bool bSuccess = false;
    UPROPERTY()
    FString FailReason;
    UPROPERTY()
    AActor* InterruptAppliedTo = nullptr;
    UPROPERTY()
    AActor* InterruptAppliedBy = nullptr;
    UPROPERTY()
    UCombatAbility* InterruptedAbility = nullptr;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY()
    UObject* InterruptSource = nullptr;
    UPROPERTY()
    float InterruptedCastStart = 0.0f;
    UPROPERTY()
    float InterruptedCastEnd = 0.0f;
    UPROPERTY()
    float InterruptTime = 0.0f;
    UPROPERTY()
    int32 ElapsedTicks = 0;
};

USTRUCT(BlueprintType)
struct FGlobalCooldown
{
    GENERATED_BODY()

    UPROPERTY()
    bool bGlobalCooldownActive = false;
    UPROPERTY()
    int32 CastID = 0;
    UPROPERTY()
    float StartTime = 0.0f;
    UPROPERTY()
    float EndTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FCastingState
{
    GENERATED_BODY()

    UPROPERTY()
    bool bIsCasting = false;
    UPROPERTY()
    UCombatAbility* CurrentCast = nullptr;
    UPROPERTY()
    int32 CurrentCastID = 0;
    UPROPERTY()
    float CastStartTime = 0.0f;
    UPROPERTY()
    float CastEndTime = 0.0f;
    UPROPERTY()
    bool bInterruptible = true;
    UPROPERTY()
    int32 ElapsedTicks = 0;
};

USTRUCT()
struct FCastRequest
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<UCombatAbility> AbilityClass;
    UPROPERTY()
    int32 CastID = 0;
    UPROPERTY()
    FGameplayTagContainer PredictedCostTags;
    UPROPERTY()
    bool bPredictedGCD = false;
    UPROPERTY()
    bool bPredictedCharges = false;
    UPROPERTY()
    bool bPredictedCastBar = false;
    UPROPERTY()
    TArray<FAbilityFloatParam> AbilityFloatParams;
    UPROPERTY()
    TArray<FAbilityPointerParam> AbilityPointerParams;
    UPROPERTY()
    TArray<FAbilityTagParam> AbilityTagParams;
};

USTRUCT()
struct FTickRequest
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<UCombatAbility> AbilityClass;
    UPROPERTY()
    int32 CastID = 0;
    UPROPERTY()
    int32 Tick = 0;
    UPROPERTY()
    TArray<FAbilityFloatParam> AbilityFloatParams;
    UPROPERTY()
    TArray<FAbilityPointerParam> AbilityPointerParams;
    UPROPERTY()
    TArray<FAbilityTagParam> AbilityTagParams;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FAbilityModCondition, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityInstanceCallback, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCallback, FCastEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCancelCallback, FCancelEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FInterruptCallback, FInterruptEvent, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAbilityChargeCallback, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGlobalCooldownCallback, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCastingStateCallback, FCastingState const&, OldState, FCastingState const&, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityNotification, FCastEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityCancelNotification, FCancelEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterruptNotification, FInterruptEvent, InterruptEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityChargeNotification, UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInstanceNotification, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGlobalCooldownNotification, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCastingStateNotification, FCastingState const&, OldState, FCastingState const&, NewState);