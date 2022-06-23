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

UENUM(BlueprintType)
enum class EThreatModifierType : uint8
{
	None = 0,
	Incoming = 1,
	Outgoing = 2,
};