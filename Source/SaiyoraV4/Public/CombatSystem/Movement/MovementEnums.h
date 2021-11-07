// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "MovementEnums.generated.h"

UENUM(BlueprintType)
enum class EMovementDirection2D : uint8
{
	None = 0,
	Forward = 1,
	Backward = 2,
	Left = 3,
	Right = 4,
	ForwardLeft = 5,
	ForwardRight = 6,
	BackwardLeft = 7,
	BackwardRight = 8,
};

UENUM(BlueprintType)
enum class ESaiyoraCustomMove : uint8
{
	None = 0,
	Teleport = 1,
	Launch = 2,
	RootMotion = 3,
};

UENUM()
enum class ERestrictableMove : uint8
{
	None = 0,
	Walk = 1,
	Jump = 2,
	Crouch = 3,
	Launch = 4,
	Teleport = 5,
	RootMotion = 6,
};