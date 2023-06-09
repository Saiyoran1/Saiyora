#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/TargetPoint.h"
#include "NPCStructs.generated.h"

class UCombatAbility;
class UAbilityCondition;

USTRUCT(BlueprintType)
struct FAbilityChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UAbilityCondition>> AbilityConditions;
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

USTRUCT(BlueprintType)
struct FCombatPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Phase"))
	FGameplayTag PhaseTag;
	UPROPERTY(EditAnywhere)
	TArray<FAbilityChoice> AbilityChoices;
	UPROPERTY(EditAnywhere)
	bool bHighPriority = false;

	bool operator==(const FCombatPhase& Other) const { return Other.PhaseTag.MatchesTagExact(PhaseTag); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);