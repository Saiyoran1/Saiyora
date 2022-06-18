#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MovementEnums.h"
#include "CombatAbility.h"
#include "MovementStructs.generated.h"

USTRUCT()
struct FCustomMoveParams
{
	GENERATED_BODY()

	UPROPERTY()
	ESaiyoraCustomMove MoveType = ESaiyoraCustomMove::None;
	UPROPERTY()
	FVector Target = FVector::ZeroVector;
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY()
	bool bStopMovement = false;
	UPROPERTY()
	bool bIgnoreRestrictions = false;
};

USTRUCT()
struct FClientPendingCustomMove
{
	GENERATED_BODY()

	TSubclassOf<UCombatAbility> AbilityClass;
	int32 PredictionID = 0;
	float OriginalTimestamp = 0.0f;
	FCustomMoveParams MoveParams;
	FAbilityOrigin Origin;
	TArray<FAbilityTargetSet> Targets;
	

	void Clear() { AbilityClass = nullptr; PredictionID = 0; OriginalTimestamp = 0.0f; MoveParams = FCustomMoveParams(); Targets.Empty(); }
};