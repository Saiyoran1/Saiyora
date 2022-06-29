#pragma once
#include "CoreMinimal.h"
#include "WeaponStructs.generated.h"

class AWeapon;

USTRUCT()
struct FFillerStruct
{
	GENERATED_BODY()
	//I don't think the generated file is created with only a multicast delegate created in the file.
	int32 Dummy = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponChanged, const bool, bPrimary, AWeapon*, PreviousWeapon, AWeapon*, NewWeapon);