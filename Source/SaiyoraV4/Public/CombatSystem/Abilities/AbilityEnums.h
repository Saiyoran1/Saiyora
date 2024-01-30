#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.generated.h"

UENUM(BlueprintType)
enum class EAbilityCastType : uint8
{
    None = 0,
    Instant = 1,
    Channel = 2,
};

UENUM(BlueprintType)
enum class ECastAction : uint8
{
    Fail = 0,
    Success = 1,
    Tick = 2,
    Complete = 3,
};

UENUM()
enum class EQueueStatus : uint8
{
    Empty = 0,
    WaitForGlobal = 1,
    WaitForCast = 2,
    WaitForBoth = 3,
};

UENUM(BlueprintType)
enum class ECastFailReason : uint8
{
    None = 0,
    InvalidAbility = 1,
    Dead = 2,
    Moving = 3,
    OnGlobalCooldown = 4,
    AlreadyCasting = 5,
    NoResourceHandler = 6,
    CostsNotMet = 7,
    ChargesNotMet = 8,
    AbilityConditionsNotMet = 9,
    CustomRestriction = 10,
    CrowdControl = 11,
    NetRole = 12,
    InvalidCastType = 13,
    Queued = 14,
};

UENUM(BlueprintType)
enum class EInterruptFailReason : uint8
{
    None = 0,
    NetRole = 1,
    NotCasting = 2,
    Restricted = 3,
};

UENUM(BlueprintType)
enum class ECancelFailReason : uint8
{
    None = 0,
    NetRole = 1,
    NotCasting = 2,
};

UENUM(BlueprintType)
enum class EChargeModificationType : uint8
{
    None = 0,
    Additive = 1,
    Override = 2,
};