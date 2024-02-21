#pragma once
#include "CoreMinimal.h"
#include "NPCEnums.h"
#include "Engine/TargetPoint.h"
#include "NPCStructs.generated.h"

class UNPCAbility;
class UCombatAbility;
class AAIController;

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

DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityTokenCallback, const bool, bTokensAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityTokenNotification, const bool, bTokensAvailable);

USTRUCT()
struct FNPCAbilityToken
{
	GENERATED_BODY()

	bool bAvailable = true;
	FTimerHandle CooldownHandle;
	UPROPERTY()
	UNPCAbility* OwningInstance = nullptr;
};

USTRUCT()
struct FNPCAbilityTokens
{
	GENERATED_BODY()

	TArray<FNPCAbilityToken> Tokens;
	int32 AvailableCount = 0;
	FAbilityTokenNotification OnTokenAvailabilityChanged;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, const EPatrolSubstate, PatrolSubstate, ATargetPoint*, LastReachedPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);