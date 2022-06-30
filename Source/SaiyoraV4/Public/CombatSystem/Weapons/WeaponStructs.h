#pragma once
#include "CoreMinimal.h"
#include "WeaponStructs.generated.h"

class AWeapon;

USTRUCT()
struct FAmmo
{
	GENERATED_BODY()
	
	int32 ShotID = 0;
	int32 CurrentAmmo = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponChanged, const bool, bPrimary, AWeapon*, PreviousWeapon, AWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, const int32, PreviousAmmo, const int32, NewAmmo);