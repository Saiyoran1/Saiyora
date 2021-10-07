// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "MovementStructs.generated.h"

USTRUCT()
struct FDirectionTeleportInfo
{
	GENERATED_BODY()

	FVector Direction = FVector::ZeroVector;
	float Length = 0.0f;
	bool bShouldSweep = false;
	bool bIgnoreZ = false;
	
};

USTRUCT()
struct FLocationTeleportInfo
{
	GENERATED_BODY()

	FVector TargetLocation = FVector::ZeroVector;
	bool bShouldSweep = false;
	bool bMaintainRotation = true;
	FRotator OverrideRotation = FRotator::ZeroRotator;
};
