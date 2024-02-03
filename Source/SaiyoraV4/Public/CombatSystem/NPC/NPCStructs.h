#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
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

//A pre-condition for an NPC to select an action.
//This is intended to be inherited from and used as an FInstancedStruct for ability and movement choices.
USTRUCT()
struct FNPCActionCondition
{
	GENERATED_BODY()
	
	//Setup function for conditions to setup callbacks to determine when they are met.
	virtual void SetupConditionChecks(AActor* Owner) {}
	//For high priority tasks, when conditions are met, they can notify the owning NPCAbilityChoice to evaluate if we can now select the choice.
	FConditionUpdated OnConditionUpdated;
	//Gets whether the condition is currently met.
	bool IsConditionMet() const { return bConditionMet; }
	
	//If a condition needs to be updated on tick (such as checking distance or location), the owning NPCAbilityChoice will update it.
	bool bRequiresTick = false;
	//For conditions requiring a tick, this will be called by the owning NPCAbilityChoice.
	virtual void TickCondition(AActor* Owner, const float DeltaTime) {}

	virtual ~FNPCActionCondition() {}

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

//An option for an action an NPC can take, with conditions that could prevent this action from being selected.
USTRUCT()
struct FNPCActionChoice
{
	GENERATED_BODY()

	//Whether this choice becoming valid should interrupt lower priority actions to execute it.
	UPROPERTY(EditAnywhere)
	bool bHighPriority = false;
	//Callback for conditions to call when they are met, if this is a high priority choice.
	void UpdateOnConditionMet();
	FOnHighPriorityChoiceAvailable OnChoiceAvailable;
	//Conditions for this choice to be selected.
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCActionCondition"))
	TArray<FInstancedStruct> ChoiceConditions;

	//Set by the NPCAbilityComponent, so that when a high priority choice becomes available, the NPCAbilityComponent can identify it.
	int32 ChoiceIndex = -1;

	void SetupConditions(AActor* Owner);
	bool AreConditionsMet() const;
	void TickConditions(AActor* Owner, const float DeltaTime);

	virtual ~FNPCActionChoice() {}
};

//An option in an NPC's ability priority. NPCs choose abilities from a stack rank of choices, each with individual conditions.
//Multiple choices can have the same ability class but different conditions.
USTRUCT()
struct FNPCAbilityChoice : public FNPCActionChoice
{
	GENERATED_BODY()

	//The ability class to use if this choice is selected.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UNPCAbility> AbilityClass;
	//The instance of the ability for use when selecting this choice.
	UPROPERTY()
	UNPCAbility* AbilityInstance = nullptr;
};

//An option for NPC movement. Can be provided by an ability choice or by an NPC's stack rank of movement options when no ability-defined movement is available.
//This struct is currently empty and intended to be inherited.
USTRUCT()
struct FNPCMovementChoice : public FNPCActionChoice
{
	GENERATED_BODY()

	virtual bool ExecuteMovementChoice(AAIController* AIController) { return false; }

	virtual ~FNPCMovementChoice() {}
};

USTRUCT(BlueprintType)
struct FNPCMovementWorldLocation : public FNPCMovementChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WorldLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Tolerance = 100.0f;

	virtual bool ExecuteMovementChoice(AAIController* AIController) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, const EPatrolSubstate, PatrolSubstate, ATargetPoint*, LastReachedPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);