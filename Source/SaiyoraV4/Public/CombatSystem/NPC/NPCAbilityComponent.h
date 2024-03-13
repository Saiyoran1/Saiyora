#pragma once
#include <NPCEnums.h>
#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "DungeonGameState.h"
#include "NPCStructs.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NPCAbilityComponent.generated.h"

class ACombatLink;
class UBehaviorTree;
class UThreatHandler;
class UCombatStatusComponent;
class APatrolRoute;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UNPCAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:

	UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FCombatBehaviorNotification OnCombatBehaviorChanged;
	UFUNCTION(BlueprintPure)
	ENPCCombatBehavior GetCombatBehavior() const { return CombatBehavior; }

protected:

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	UPROPERTY()
	AAIController* AIController = nullptr;
	void OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& PathResult);
	UPROPERTY()
	ADungeonGameState* DungeonGameStateRef = nullptr;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_CombatBehavior)
	ENPCCombatBehavior CombatBehavior = ENPCCombatBehavior::None;
	UFUNCTION()
	void OnRep_CombatBehavior(const ENPCCombatBehavior Previous);
	void UpdateCombatBehavior();
	UFUNCTION()
	void OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController);
	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase) { UpdateCombatBehavior(); }
	UFUNCTION()
	void OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus) { UpdateCombatBehavior(); }
	UFUNCTION()
	void OnCombatChanged(UThreatHandler* Handler, const bool bInCombat) { UpdateCombatBehavior(); }

#pragma region Combat

private:
	
	//The behavior tree to run for combat for this actor.
	UPROPERTY(EditAnywhere, Category = "Combat")
	UBehaviorTree* CombatTree = nullptr;

	void EnterCombatState();
	void LeaveCombatState();

#pragma endregion 
#pragma region Patrolling

public:

	UPROPERTY(BlueprintAssignable)
	FPatrolLocationNotification OnPatrolLocationReached;
	UPROPERTY(BlueprintAssignable)
	FPatrolStateNotification OnPatrolStateChanged;

	void SetupGroupPatrol(ACombatLink* LinkActor);

private:

	UPROPERTY(EditAnywhere, Category = "Patrol")
	APatrolRoute* PatrolRoute = nullptr;
	//Whether this NPC should loop the patrol path once they reach the last point, or just stay there.
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bLoopPatrol = true;
	//If the NPC is looping the patrol path, this determines whether they will patrol in reverse, or go straight from the end to start and restart the path.
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (EditCondition = "bLoopPatrol"))
	bool bLoopReverse = true;
	//Optional move speed modifier applied to the NPC when in the patrol state.
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolMoveSpeedModifier = 0.0f;

	void EnterPatrolState();
	void LeavePatrolState();

	void SetPatrolSubstate(const EPatrolSubstate NewSubstate);

	void MoveToNextPatrolPoint();
	void OnReachedPatrolPoint();
	void FinishPatrol();

	EPatrolSubstate PatrolSubstate = EPatrolSubstate::None;
	TArray<FPatrolPoint> PatrolPath;
	int32 NextPatrolIndex = 0;
	int32 LastPatrolIndex = -1;
	void IncrementPatrolIndex();
	bool bReversePatrolling = false;
	bool bGroupPatrolling = false;
	UPROPERTY()
	ACombatLink* GroupLink = nullptr;
	UFUNCTION()
	void FinishGroupPatrolSegment();

	FTimerHandle PatrolWaitHandle;
	UFUNCTION()
	void FinishPatrolWait();
	
	FCombatModifierHandle PatrolMoveSpeedModHandle;
	float DefaultMaxWalkSpeed = 0.0f;

	FAIRequestID CurrentMoveRequestID = FAIRequestID::InvalidRequest;

#pragma endregion 
#pragma region Resetting

public:

private:

	void EnterResetState();
	void LeaveResetState();
	
	bool bNeedsReset = false;
	FVector ResetGoal;

	//If we are further than this away from our reset goal, we will teleport instead of pathing back.
	static constexpr float ResetTeleportDistance = 2000.0f;

#pragma endregion
#pragma region Tokens

private:

	UFUNCTION()
	void UpdateAbilityTokensOnCastStateChanged(const FCastingState& PreviousState, const FCastingState& NewState);

#pragma endregion
#pragma region Test Ability Priority

public:

	void OnChoiceBecameValid(const int ChoiceIdx);

private:

	UPROPERTY(EditAnywhere, Category = "TEST")
	TArray<FNPCCombatChoice> CombatPriority;

	void InitCombatChoices();
	void TrySelectNewChoice();
	void StartExecuteChoice();
	
	void AbortCurrentChoice();
	int CurrentCombatChoiceIdx = -1;
	ENPCCombatChoiceStatus CombatChoiceStatus = ENPCCombatChoiceStatus::None;
	bool bInitializedChoices = false;

	void RunQuery();
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult);
	UPROPERTY()
	UEnvQuery* CurrentQuery = nullptr;
	TArray<FAIDynamicParam> CurrentQueryParams;
	int32 QueryID = INDEX_NONE;

	bool bHasValidMoveLocation = false;
	FVector CurrentMoveLocation;

#pragma endregion 
};
