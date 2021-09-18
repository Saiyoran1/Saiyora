// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EThreatType : uint8
{
	None = 0,
	Absolute = 1,
	Damage = 2,
	Healing = 3,
};

UENUM(BlueprintType)
enum class EThreatAction : uint8
{
	None = 0,
	Fixate = 1,
	Blind = 2,
	Fade = 3,
	Taunt = 4,
	Drop = 5,
	Vanish = 6,
	Transfer = 7,
	Misdirect = 8,
};