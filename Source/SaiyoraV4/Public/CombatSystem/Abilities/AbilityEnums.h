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