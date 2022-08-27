#pragma once
#include "CoreMinimal.h"
#include "WeaponEnums.generated.h"

UENUM(BlueprintType)
enum class EWeaponFailReason : uint8
{
	None = 0,
	InvalidWeapon = 1,
	Dead = 2,
	FireRate = 3,
	NotEnoughAmmo = 4,
	WrongPlane = 5,
	WeaponConditionsNotMet = 6,
	CustomRestriction = 7,
	CrowdControl = 8,
	Casting = 9,
	NetRole = 10,
	Queued = 11,
};
