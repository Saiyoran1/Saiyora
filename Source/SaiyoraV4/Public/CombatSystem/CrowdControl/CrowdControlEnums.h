#pragma once
#include "CoreMinimal.h"
#include "CrowdControlEnums.generated.h"

UENUM()
enum class ECrowdControlType : uint8
{
	None,
	Stun,
	Incapacitate,
	Root,
	Silence,
	Disarm,
};
