// Fill out your copyright notice in the Description page of Project Settings.

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
    Fail,
    Success,
    Que,
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

UENUM()
enum class EActionBarType : uint8
{
    None = 0,
    Ancient = 1,
    Modern = 2,
    Hidden = 3,
};

UENUM(BlueprintType)
enum class ECastFailReason : uint8
{
    None = 0,
    InvalidAbility = 1,
    Dead = 2,
    OnGlobalCooldown = 3,
    AlreadyCasting = 4,
    NoResourceHandler = 5,
    CostsNotMet = 6,
    ChargesNotMet = 7,
    AbilityConditionsNotMet = 8,
    CustomRestriction = 9,
    CrowdControl = 10,
    NetRole = 11,
    InvalidCastType = 12,
    Queued = 13,
};

UENUM(BlueprintType)
enum class EInterruptFailReason : uint8
{
    None = 0,
    NetRole = 1,
    NotCasting = 2,
    Restricted = 3,
};

UENUM()
enum class EAbilityPermission : uint8
{
    None = 0, //Simulated proxy.
    Auth = 1, //Server with remote role of AutoProxy.
    AuthPlayer = 2, //Listen server.
    PredictPlayer = 3, //Auto proxy.
};