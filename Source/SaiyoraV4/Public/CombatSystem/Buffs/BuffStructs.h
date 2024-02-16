#pragma once
#include "BuffEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "InstancedStruct.h"
#include "BuffStructs.generated.h"

class UBuff;

//Struct detailing a buff application event.
USTRUCT(BlueprintType)
struct FBuffApplyEvent
{
    GENERATED_BODY()

    //Whether we created a new buff, stacked/refreshed an existing one, or failed to apply
    UPROPERTY(BlueprintReadOnly)
    EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
    //All of the reasons we couldn't apply a buff
    UPROPERTY(BlueprintReadOnly)
    TArray<EBuffApplyFailReason> FailReasons;

    //The class of buff we tried to apply
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> BuffClass;

    //The actor the buff was applied to
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedTo = nullptr;
    //The plane of the actor the buff was applied to, at application time
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane TargetPlane = ESaiyoraPlane::Both;

    //The actor the buff was applied by
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedBy = nullptr;
    //The plane of the actor the buff was applied by, at application time
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane OriginPlane = ESaiyoraPlane::Both;

    //The source of the buff, usually an ability, but can be anything
    UPROPERTY(BlueprintReadOnly)
    UObject* Source = nullptr;
    //Any custom parameters passed in for the buff to use
    UPROPERTY(BlueprintReadOnly, meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
    TArray<FInstancedStruct> CombatParams;

    //The instance of the buff that was either created or stacked/refreshed
    UPROPERTY(BlueprintReadOnly)
    UBuff* AffectedBuff = nullptr;
    //The buff instance's stacks before this event
    UPROPERTY(BlueprintReadOnly)
    int32 PreviousStacks = 0;
    //The buff instance's stacks after this event
    UPROPERTY(BlueprintReadOnly)
    int32 NewStacks = 0;
    //The buff instance's remaining duration before this event
    UPROPERTY(BlueprintReadOnly)
    float PreviousDuration = 0.0f;
    //The buff instance's remaining duration after this event
    UPROPERTY(BlueprintReadOnly)
    float NewDuration = 0.0f;
    //Timestamp of the buff application event
    UPROPERTY(BlueprintReadOnly)
    float NewApplyTime = 0.0f;
};

//Struct detailing the removal of a buff
USTRUCT(BlueprintType)
struct FBuffRemoveEvent
{
    GENERATED_BODY()

    //The buff instance that was removed
    UPROPERTY(BlueprintReadOnly)
    UBuff* RemovedBuff = nullptr;
    //The actor that removed this buff
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedBy = nullptr;
    //The actor the buff was applied to before being removed
    UPROPERTY(BlueprintReadOnly)
    AActor* RemovedFrom = nullptr;
    //A generic reason for why the buff was removed
    UPROPERTY(BlueprintReadOnly)
    EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;
    //Whether the buff was successfully removed or not
    UPROPERTY(BlueprintReadOnly)
    bool Result = false;
};

//Restriction function for conditionally preventing buffs from being applied
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBuffRestriction, const FBuffApplyEvent&, BuffEvent);
//Multicast delegate for notifying when buffs are applied
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffEventNotification, const FBuffApplyEvent&, BuffEvent);
//Multicast delegate for notifying when buffs are removed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffRemoveNotification, const FBuffRemoveEvent&, RemoveEvent);
