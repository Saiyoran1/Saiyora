#pragma once
#include "BuffEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "InstancedStruct.h"
#include "BuffStructs.generated.h"

class UBuff;

USTRUCT(BlueprintType)
struct FBuffApplyEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
    UPROPERTY(BlueprintReadOnly)
    TArray<EBuffApplyFailReason> FailReasons;
    
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> BuffClass;
    
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane TargetPlane = ESaiyoraPlane::Both;
    
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane OriginPlane = ESaiyoraPlane::Both;
    
    UPROPERTY(BlueprintReadOnly)
    UObject* Source = nullptr;
    UPROPERTY(BlueprintReadOnly, meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
    TArray<FInstancedStruct> CombatParams;
    
    UPROPERTY(BlueprintReadOnly)
    UBuff* AffectedBuff = nullptr;
    UPROPERTY(BlueprintReadOnly)
    int32 PreviousStacks = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 NewStacks = 0;
    UPROPERTY(BlueprintReadOnly)
    float PreviousDuration = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float NewDuration = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float NewApplyTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FBuffRemoveEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    UBuff* RemovedBuff = nullptr;
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly)
    AActor* RemovedFrom = nullptr;
    UPROPERTY(BlueprintReadOnly)
    EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;
    UPROPERTY(BlueprintReadOnly)
    bool Result = false;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBuffRestriction, const FBuffApplyEvent&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffEventCallback, const FBuffApplyEvent&, BuffEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffEventNotification, const FBuffApplyEvent&, BuffEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBuffRemoveCallback, const FBuffRemoveEvent&, RemoveEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffRemoveNotification, const FBuffRemoveEvent&, RemoveEvent);
