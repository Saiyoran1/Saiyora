#pragma once
#include "CoreMinimal.h"
#include "CrowdControlEnums.generated.h"

UENUM(BlueprintType)
enum class ECrowdControl : uint8
{
	Stun,
	Incapacitate,
	Root,
	Silence,
	Disarm
};

ENUM_RANGE_BY_FIRST_AND_LAST(ECrowdControl, ECrowdControl::Stun, ECrowdControl::Disarm);
