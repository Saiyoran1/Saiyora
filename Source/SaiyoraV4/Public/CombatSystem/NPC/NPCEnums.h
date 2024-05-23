#pragma once
#include "CoreMinimal.h"
#include "NPCEnums.generated.h"

UENUM(BlueprintType)
enum class ENPCCombatBehavior : uint8
{
	None,
	Patrolling,
	Combat,
	Resetting,
};

UENUM(BlueprintType)
enum class ENPCPatrolSubstate : uint8
{
	None,
	MovingToPoint,
	WaitingAtPoint,
	PatrolFinished
};

UENUM()
enum class ENPCCombatSubstate : uint8
{
	None,
	PreMoveQuery,
	PreMoving,
	Casting,
};