// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EThreatActionType : uint8
{
	None = 0,
	Taunt = 1, //Move the ToThreat player up to a >100% percentage of the current highest threat's threat.
	Drop = 2, //Drop a percentage of the FromThreat player's threat.
	Transfer = 3, //Transfer a percentage of the FromThreat player's threat to the ToThreat player.
};

UENUM(BlueprintType)
enum class EThreatControlType : uint8
{
	None = 0,
	Fixate = 1, //Ignore highest threat targets for the duration, only attack ToThreat player.
	Ignore = 2, //Ignore the FromThreat player for the duration, preventing them from being the target regardless of threat.
	Misdirect = 3, //Any threat taken from the FromThreat player is added as threat for the ToThreat player.
};