#pragma once
#include "CoreMinimal.h"
#include "MovementEnums.generated.h"

UENUM(BlueprintType)
enum class ESaiyoraCustomMove : uint8
{
	None = 0,
	Teleport = 1,
	Launch = 2,
};

UENUM()
enum class ERestrictableMove : uint8
{
	None = 0,
	Walk = 1,
	Jump = 2,
	Crouch = 3,
	Launch = 4,
	Teleport = 5,
	RootMotion = 6,
};