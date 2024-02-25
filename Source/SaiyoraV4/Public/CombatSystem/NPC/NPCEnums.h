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

UENUM()
enum class ENPCCombatChoiceStatus : uint8
{
	None,
	PreMoving,
	Casting,
};