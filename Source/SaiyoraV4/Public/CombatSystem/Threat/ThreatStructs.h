#pragma once
#include "CoreMinimal.h"
#include "ThreatEnums.h"
#include "SaiyoraStructs.h"
#include "DamageStructs.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FThreatCondition, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FThreatModCondition, FDamagingEvent const&, DamageEvent);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCombatStatusCallback, bool const, NewCombatStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatStatusNotification, bool const, NewCombatStatus);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FTargetCallback, AActor*, PreviousTarget, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTargetNotification, AActor*, PreviousTarget, AActor*, NewTarget);

USTRUCT()
struct FThreatControl
{
	GENERATED_BODY()

	EThreatControlType ControlType = EThreatControlType::None;
	UPROPERTY()
	class UBuff* Source = nullptr;
	UPROPERTY()
	AActor* ThreatFrom = nullptr;
	UPROPERTY()
	AActor* ThreatTo = nullptr;
};

USTRUCT()
struct FThreatAction
{
	GENERATED_BODY()
	
	EThreatActionType ActionType = EThreatActionType::None;
	UPROPERTY()
	AActor* ThreatFrom = nullptr;
	UPROPERTY()
	AActor* ThreatTo = nullptr;
	float Percentage = 0.0f;
};

USTRUCT()
struct FThreatTarget
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Unit;
	float Threat = 0.0f;
	TArray<FThreatControl> Controls;
};