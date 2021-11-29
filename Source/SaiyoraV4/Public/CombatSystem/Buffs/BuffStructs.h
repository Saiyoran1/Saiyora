#pragma once
#include "BuffEnums.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "BuffStructs.generated.h"

class UBuff;

USTRUCT(BlueprintType)
struct FBuffApplyEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TSubclassOf<class UBuff> BuffClass;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    AActor* AppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    UObject* Source = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane TargetPlane = ESaiyoraPlane::None; 
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    ESaiyoraPlane OriginPlane = ESaiyoraPlane::None;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    bool AppliedXPlane = false;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    TArray<FCombatParameter> CombatParams;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    UBuff* AffectedBuff = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 PreviousStacks = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    int32 NewStacks = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    float PreviousDuration = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    float NewDuration = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    float NewApplyTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FBuffRemoveEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    UBuff* RemovedBuff = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    AActor* RemovedFrom = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;
    UPROPERTY(BlueprintReadOnly, Category = "Buff")
    bool Result = false;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBuffRestriction, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffEventCallback, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffEventNotification, FBuffApplyEvent const&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffRemoveCallback, FBuffRemoveEvent const&, RemoveEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffRemoveNotification, FBuffRemoveEvent const&, RemoveEvent);
DECLARE_DELEGATE(FModifierCallback);
DECLARE_MULTICAST_DELEGATE(FModifierNotification);
