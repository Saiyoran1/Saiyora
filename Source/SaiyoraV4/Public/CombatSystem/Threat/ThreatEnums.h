#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EThreatType : uint8
{
	None = 0,
	Absolute = 1,
	Damage = 2,
	Healing = 3,
};