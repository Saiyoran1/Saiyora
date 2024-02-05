#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.generated.h"

UENUM(BlueprintType)
enum class EAbilityCastType : uint8
{
    Instant,
    Channel,
};

UENUM(BlueprintType)
enum class ECastAction : uint8
{
    Fail,
    Success,
    Tick,
    Complete,
};

UENUM()
enum class EQueueStatus : uint8
{
    Empty,
    WaitForGlobal,
    WaitForCast,
    WaitForBoth,
};

UENUM(BlueprintType)
enum class ECastFailReason : uint8
{
    None,
    InvalidAbility,
    Dead,
    Moving,
    OnGlobalCooldown,
    AlreadyCasting,
    NoResourceHandler,
    CostsNotMet,
    ChargesNotMet,
    AbilityConditionsNotMet,
    CustomRestriction,
    CrowdControl,
    NetRole,
    Queued,
};

UENUM(BlueprintType)
enum class EInterruptFailReason : uint8
{
    None,
    NetRole,
    NotCasting,
    Restricted,
};

UENUM(BlueprintType)
enum class ECancelFailReason : uint8
{
    None,
    NetRole,
    NotCasting,
};

UENUM(BlueprintType)
enum class EChargeModificationType : uint8
{
    Additive,
    Override,
};