#pragma once
#include "CoreMinimal.h"
#include "NPCEnums.h"
#include "Engine/TargetPoint.h"
#include "NPCStructs.generated.h"

class UCombatAbility;

UENUM(BlueprintType)
enum class EPatrolSubstate : uint8
{
	None,
	MovingToPoint,
	WaitingAtPoint,
	PatrolFinished
};

USTRUCT(BlueprintType)
struct FPatrolPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ATargetPoint* Point = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WaitTime = 0.0f;
};

USTRUCT()
struct FNPCAbilityLocationRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	ELocationRuleReferenceType LocationRuleType = ELocationRuleReferenceType::None;
	UPROPERTY()
	AActor* RangeTargetActor = nullptr;
	UPROPERTY()
	FVector RangeTargetLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None"))
	bool bEnforceMinimumRange = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None && bEnforceMinimumRange"))
	float MinimumRange = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None"))
	bool bEnforceMaximumRange = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None && bEnforceMaximumRange"))
	float MaximumRange = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, const EPatrolSubstate, PatrolSubstate, ATargetPoint*, LastReachedPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);