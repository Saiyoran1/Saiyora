// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BuffEnums.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "BuffStructs.generated.h"

class UBuff;

USTRUCT(BlueprintType)
struct FBuffApplyResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    UBuff* AffectedBuff = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    int32 PreviousStacks = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    int32 NewStacks = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    float PreviousDuration = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    float NewApplyTime = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
    float NewDuration = 0.0f;
};

USTRUCT(BlueprintType)
struct FBuffApplyEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    AActor* AppliedTo = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    AActor* AppliedBy = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    UObject* Source = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool AppliedXPlane = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None; 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None; 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    TSubclassOf<class UBuff> BuffClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool DuplicateOverride = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    EBuffApplicationOverrideType StackOverrideType = EBuffApplicationOverrideType::None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 OverrideStacks = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    EBuffApplicationOverrideType RefreshOverrideType = EBuffApplicationOverrideType::None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    float OverrideDuration = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    TArray<FCombatParameter> CombatParams;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    FBuffApplyResult Result;
};

USTRUCT(BlueprintType)
struct FBuffRemoveEvent
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    UBuff* RemovedBuff = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    AActor* AppliedBy = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    AActor* RemovedFrom = nullptr;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool Result = false;
};

USTRUCT(BlueprintType)
struct FBuffInitialState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 Stacks = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    float ApplyTime = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    float ExpireTime = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    bool XPlane = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None; 
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBuffEventCondition, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffEventCallback, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffEventNotification, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffRemoveCallback, FBuffRemoveEvent const&, RemoveEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffRemoveNotification, FBuffRemoveEvent const&, RemoveEvent);
DECLARE_DELEGATE(FModifierCallback);
DECLARE_MULTICAST_DELEGATE(FModifierNotification);
