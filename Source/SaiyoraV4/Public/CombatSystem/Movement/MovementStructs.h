// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MovementEnums.h"
#include "PlayerCombatAbility.h"
#include "SaiyoraStructs.h"
#include "MovementStructs.generated.h"

USTRUCT()
struct FCustomMoveParams
{
	GENERATED_BODY()

	FVector Target = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	float Scale = 0.0f;
	FRotator Rotation = FRotator::ZeroRotator;
	FGameplayTag MoveStatTag;
	FCombatModifier StatModifier;
	bool bSweep = false;
	bool bIgnoreZ = false;
};

USTRUCT()
struct FClientPendingCustomMove
{
	GENERATED_BODY()

	TSubclassOf<UPlayerCombatAbility> AbilityClass;
	int32 PredictionID = 0;
	float OriginalTimestamp = 0.0f;
	ESaiyoraCustomMove MoveType;
	FCustomMoveParams MoveParams;
	FCombatParameters PredictionParams;

	void Clear() { AbilityClass = nullptr; PredictionID = 0; MoveType = ESaiyoraCustomMove::None; MoveParams = FCustomMoveParams(); PredictionParams = FCombatParameters(); }
};
