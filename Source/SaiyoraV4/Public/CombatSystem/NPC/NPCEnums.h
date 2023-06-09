#pragma once
#include "CoreMinimal.h"
#include "NPCEnums.generated.h"

UENUM(BlueprintType)
enum class ENPCCombatBehavior : uint8
{
	None = 0,
	Patrolling = 1,
	Combat = 2,
	Resetting = 3,
};