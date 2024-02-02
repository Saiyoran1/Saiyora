#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "NPCEnums.h"
#include "Engine/TargetPoint.h"
#include "NPCStructs.generated.h"

class UNPCAbility;
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

//Used for NPC Abilities to be queried by the NPC using them to find out what kind of positioning requirements the ability needs.
USTRUCT()
struct FNPCAbilityLocationRules
{
	GENERATED_BODY()

	//What kind of rule this ability has:
	//None: No requirement at all.
	//Actor: Min/max range from a specified actor.
	//Location: Min/max range from a specified location.
	UPROPERTY(EditAnywhere)
	ELocationRuleReferenceType LocationRuleType = ELocationRuleReferenceType::None;
	//If using Actor rule type, this is the actor to check range against.
	UPROPERTY()
	AActor* RangeTargetActor = nullptr;
	//If using Location rule type, this is the location to check range against.
	UPROPERTY()
	FVector RangeTargetLocation = FVector::ZeroVector;
	//Whether this ability has a minimum range.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None"))
	bool bEnforceMinimumRange = false;
	//Minimum range of the ability.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None && bEnforceMinimumRange"))
	float MinimumRange = 0.0f;
	//Whether this ability has a maximum range.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None"))
	bool bEnforceMaximumRange = false;
	//Maximum range of the ability.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "LocationRuleType != ELocationRuleReferenceType::None && bEnforceMaximumRange"))
	float MaximumRange = 0.0f;
};

DECLARE_DELEGATE(FConditionUpdated);

//A pre-condition for an NPC to select an ability as its next action.
//This is intended to be inherited from and used as an FInstancedStruct.
USTRUCT()
struct FNPCAbilityCondition
{
	GENERATED_BODY()

	//Setup function for conditions to setup callbacks to determine when they are met.
	virtual void SetupConditionChecks(AActor* Owner, const TSubclassOf<UNPCAbility> AbilityClass) {}
	//For high priority tasks, when conditions are met, they can notify the owning NPCAbilityChoice to evaluate if we can now select the choice.
	FConditionUpdated OnConditionUpdated;
	//Gets whether the condition is currently met.
	bool IsConditionMet() const { return bConditionMet; }
	
	//If a condition needs to be updated on tick (such as checking distance or location), the owning NPCAbilityChoice will update it.
	bool bRequiresTick = false;
	//For conditions requiring a tick, this will be called by the owning NPCAbilityChoice.
	virtual void TickCondition(AActor* Owner, const TSubclassOf<UNPCAbility> AbilityClass, const float DeltaTime) {}

	virtual ~FNPCAbilityCondition() {}

protected:

	//For derived structs to update when the condition is met.
	void UpdateConditionMet(const bool bMet)
	{
		bConditionMet = bMet;
		if (bConditionMet)
		{
			OnConditionUpdated.ExecuteIfBound();
		}
	}

private:

	//Whether this condition is currently met.
	bool bConditionMet = false;
};

DECLARE_DELEGATE_OneParam(FOnHighPriorityChoiceAvailable, const int32);

//An option in an NPC's ability priority. NPCs using the generic combat tree decide what ability to use next from a stack ranking of NPCAbilityChoices.
//Multiple choices can have the same ability class but different conditions.
USTRUCT()
struct FNPCAbilityChoice
{
	GENERATED_BODY()

	//The ability class to use if this choice is selected.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UNPCAbility> AbilityClass;
	//The instance of the ability for use when selecting this choice.
	UPROPERTY()
	UNPCAbility* AbilityInstance = nullptr;
	//Whether this choice becoming valid should interrupt all actions to prioritize it.
	UPROPERTY(EditAnywhere)
	bool bHighPriority = false;
	//Callback for conditions to call when they are met, if this is a high priority choice.
	void UpdateOnConditionMet();
	FOnHighPriorityChoiceAvailable OnChoiceAvailable;
	//Conditions for this choice to be selected.
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCAbilityCondition"))
	TArray<FInstancedStruct> ChoiceConditions;

	//Set by the NPCAbilityComponent, so that when a high priority choice becomes available, the NPCAbilityComponent can identify it.
	int32 ChoiceIndex = -1;

	void SetupConditions(AActor* Owner);
	bool AreConditionsMet() const;
	void TickConditions(AActor* Owner, const float DeltaTime);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, const EPatrolSubstate, PatrolSubstate, ATargetPoint*, LastReachedPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);