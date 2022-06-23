#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "PlaneStructs.generated.h"

class UPlaneComponent;

USTRUCT()
struct FPlaneStatus
{
	GENERATED_BODY()

	UPROPERTY()
	ESaiyoraPlane CurrentPlane = ESaiyoraPlane::None;
	UPROPERTY()
	UObject* LastSwapSource = nullptr;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_FourParams(bool, FPlaneSwapRestriction, UPlaneComponent*, Target, UObject*, Source, const bool, bToSpecificPlane, const ESaiyoraPlane, TargetPlane);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlaneSwapNotification, const ESaiyoraPlane, PreviousPlane, const ESaiyoraPlane, NewPlane, UObject*, Source);