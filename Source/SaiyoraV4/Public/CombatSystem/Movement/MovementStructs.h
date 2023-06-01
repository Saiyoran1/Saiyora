#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "MovementEnums.h"
#include "MovementStructs.generated.h"

class UCombatAbility;

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
struct FServerWaitingCustomMove
{
	GENERATED_BODY();
	
	FCustomMoveParams MoveParams;
	FTimerHandle TimerHandle;

	FServerWaitingCustomMove() {}
	FServerWaitingCustomMove(const FCustomMoveParams& Move) : MoveParams(Move) {}
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementChanged, AActor*, Actor, const bool, bNewMovement);