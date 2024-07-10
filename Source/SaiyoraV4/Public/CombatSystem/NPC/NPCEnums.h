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

UENUM(BlueprintType)
enum class ENPCAbilityTokenState : uint8
{
	Available,
	Reserved,
	InUse,
	Cooldown
};