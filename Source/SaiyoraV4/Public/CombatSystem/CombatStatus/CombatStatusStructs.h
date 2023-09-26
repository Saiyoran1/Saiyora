#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "CombatStatusStructs.generated.h"

class UCombatStatusComponent;

USTRUCT()
struct FPlaneStatus
{
	GENERATED_BODY()

	UPROPERTY()
	ESaiyoraPlane CurrentPlane = ESaiyoraPlane::None;
	UPROPERTY()
	UObject* LastSwapSource = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlaneSwapRestrictionNotification, const bool, bRestricted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlaneSwapNotification, const ESaiyoraPlane, PreviousPlane, const ESaiyoraPlane, NewPlane, UObject*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatNameChanged, const FName, PreviousName, const FName, NewName);