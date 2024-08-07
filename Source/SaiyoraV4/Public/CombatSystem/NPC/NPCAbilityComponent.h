﻿#pragma once
#include <NPCEnums.h>
#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "DungeonGameState.h"
#include "NPCStructs.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NPCAbilityComponent.generated.h"

class UNPCSubsystem;
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	UPROPERTY()
	AAIController* AIController = nullptr;
	
	UPROPERTY()
	ADungeonGameState* DungeonGameStateRef = nullptr;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef = nullptr;
	UPROPERTY()
	UNPCSubsystem* NPCSubsystemRef = nullptr;

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


#pragma region Movement

public:

	bool HasValidMoveGoal() const { return bHasValidMoveLocation; }
	FVector GetMoveGoal() const { return CurrentMoveLocation; }

private:

	void SetMoveGoal(const FVector& Location);
	void SetWantsToMove(const bool bNewMove);
	void TryMoveToGoal();
	void AbortActiveMove();
	//Bound to the AIController's PathFollowingComponent, fires whenever move requests finish or fail.
	void OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& PathResult);
	//What behavior state the NPC was in when the last move request was sent.
	//This is to prevent reacting to move requests from combat state after we've gone to resetting/patrolling, or vice versa.
	ENPCCombatBehavior CurrentMoveRequestBehavior = ENPCCombatBehavior::None;
	//RequestID of the last requested move.
	FAIRequestID CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	bool bHasValidMoveLocation = false;
	FVector CurrentMoveLocation;
	bool bWantsToMove = false;
	
#pragma endregion
#pragma region Rotation

public:

	UFUNCTION(BlueprintCallable)
	FRotationLock LockRotation();
	UFUNCTION(BlueprintCallable)
	void UnlockRotation(const FRotationLock& Lock);

private:

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (BaseStruct = "/Script/SaiyoraV4.NPCRotationBehavior", ExcludeBaseStruct))
	FInstancedStruct DefaultRotationBehavior;

	void TickRotation(const float DeltaTime);

	TArray<FRotationLock> RotationLocks;

#pragma endregion 
#pragma region Query

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat", meta = (BaseStruct = "/Script/SaiyoraV4.NPCQueryParam"))
	void SetQuery(UEnvQuery* Query, const TArray<FInstancedStruct>& Params);

private:

	UPROPERTY(EditAnywhere, Category = "Combat")
	UEnvQuery* DefaultQuery = nullptr;
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (BaseStruct = "/Script/SaiyoraV4.NPCQueryParam", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefaultQueryParams;

	UFUNCTION()
	void RunQuery();
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult);
	void AbortActiveQuery();

	UPROPERTY()
	UEnvQuery* CurrentQuery = nullptr;
	TArray<FInstancedStruct> CurrentQueryParams;
	int32 QueryID = INDEX_NONE;
	ENPCCombatBehavior LastQueryBehavior = ENPCCombatBehavior::None;
	FTimerHandle QueryRetryHandle;
	//Base time between queries. This is used with some random variation to desync queries for multiple AI.
	static constexpr float BaseQueryRetryDelay = 0.2f;
	//Random seed used to determine variance between queries.
	int QueryRetryRandomSeed = 0;
	//Called when entering combat to get a unique seed for each enemy to calculate their random variation between queries.
	static int GenerateQueryRetryRandomSeed()
	{
		static int QueryRetrySeedCounter = 0;
		return QueryRetrySeedCounter++;
	}
	//Used when calculating delay time until next query. Alternates between adding this enemy's random variance and subtracting it from the base value.
	mutable bool bQueryRetryRandomAdd = false;
	//Get the time until the next query, using this enemy's randomized variance between queries.
	float GetQueryRetryDelay() const
	{
		bQueryRetryRandomAdd = !bQueryRetryRandomAdd;
		return BaseQueryRetryDelay + ((bQueryRetryRandomAdd ? 1.0f : -1.0f) * (QueryRetryRandomSeed % 5) * (BaseQueryRetryDelay * 0.1f));
	}

#pragma endregion 
#pragma region Combat Choices

public:

private:
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FNPCCombatChoice> CombatPriority;

	void EnterCombatState();
	void LeaveCombatState();
	void InitCombatChoices();
	UFUNCTION()
	void TrySelectNewChoice();
	UFUNCTION()
	void EndChoiceOnCastStateChanged(const FCastingState& Previous, const FCastingState& New);
	
	bool bInitializedChoices = false;
	FTimerHandle ChoiceRetryHandle;
	static constexpr float ChoiceRetryDelay = 0.5f;

	bool bWaitingOnMovementStop = false;
	int QueuedChoiceIdx = -1;
	UFUNCTION()
	void TryUseQueuedAbility(AActor* Actor, const bool bNewMovement);

	UPROPERTY()
	UNPCAbility* PossessedToken = nullptr;

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

	void SetPatrolSubstate(const ENPCPatrolSubstate NewSubstate);

	void MoveToNextPatrolPoint();
	void OnReachedPatrolPoint();
	void FinishPatrol();

	ENPCPatrolSubstate PatrolSubstate = ENPCPatrolSubstate::None;
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
};
