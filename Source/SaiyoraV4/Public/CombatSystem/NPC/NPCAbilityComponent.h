#pragma once
#include <NPCEnums.h>
#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "DungeonGameState.h"
#include "NPCStructs.h"
#include "NPCAbilityComponent.generated.h"

class UBehaviorTree;
class UThreatHandler;
class UCombatStatusComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UNPCAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

	UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer);

public:

	UPROPERTY(BlueprintAssignable)
	FCombatBehaviorNotification OnCombatBehaviorChanged;
	UFUNCTION(BlueprintPure)
	ENPCCombatBehavior GetCombatBehavior() const { return CombatBehavior; }

protected:

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Category = "Behavior")
	UBehaviorTree* BehaviorTree;

private:

	UPROPERTY()
	APawn* OwnerAsPawn;
	UPROPERTY()
	AAIController* AIController;
	UPROPERTY()
	ADungeonGameState* DungeonGameStateRef;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef;
	UPROPERTY()
	UCombatStatusComponent* CombatStatusComponentRef;

	UPROPERTY(ReplicatedUsing=OnRep_CombatBehavior)
	ENPCCombatBehavior CombatBehavior = ENPCCombatBehavior::None;
	UFUNCTION()
	void OnRep_CombatBehavior(const ENPCCombatBehavior Previous);
	void UpdateCombatBehavior();
	UFUNCTION()
	void OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController) { AIController = Cast<AAIController>(NewController); UpdateCombatBehavior(); }
	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase) { UpdateCombatBehavior(); }
	UFUNCTION()
	void OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus) { UpdateCombatBehavior(); }
	UFUNCTION()
	void OnCombatChanged(UThreatHandler* Handler, const bool bInCombat) { UpdateCombatBehavior(); }

	void SetupBehavior();
	bool bInitialized = false;

	//Combat

public:

	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	FCombatPhase GetCombatPhase() const;
	UFUNCTION(BlueprintAuthorityOnly, meta = (GameplayTagFilter = "Phase"))
	void EnterPhase(const FGameplayTag PhaseTag);

protected:

	UPROPERTY(EditAnywhere, Category = "Behavior", meta = (Categories = "Phase"))
	FGameplayTag DefaultPhase;

private:

	UPROPERTY(EditAnywhere, Category = "Behavior")
	TArray<FCombatPhase> Phases;
	FGameplayTag CurrentPhaseTag;
	static constexpr float ActionRetryFrequency = 0.5f;
	FTimerHandle ActionRetryHandle;

	void EnterCombatState();
	void LeaveCombatState();
	UFUNCTION()
	void DetermineNewAction();
	void InterruptCurrentAction();

	UFUNCTION()
	void OnCastEnded(const FCastingState& PreviousState, const FCastingState& NewState);
	UFUNCTION()
	void OnTargetChanged(AActor* PreviousTarget, AActor* NewTarget);

	//Patrolling

public:

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetNextPatrolPoint(UBlackboardComponent* Blackboard);
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void ReachedPatrolPoint(const FVector& Location);

	UPROPERTY(BlueprintAssignable)
	FPatrolLocationNotification OnPatrolLocationReached;

protected:

	UPROPERTY(EditAnywhere, Category = "Patrol")
	TArray<FPatrolPoint> Patrol;
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bLoopPatrol = true;
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolMoveSpeedModifier = 0.0f;

private:

	void EnterPatrolState();
	void LeavePatrolState();

	FCombatModifierHandle PatrolMoveSpeedModHandle;
	float DefaultMaxWalkSpeed = 0.0f;
	int32 NextPatrolIndex = 0;
	bool bFinishedPatrolling = false;

	//Resetting

public:

	UFUNCTION(BlueprintCallable)
	void MarkResetComplete();

private:

	void EnterResetState();
	void LeaveResetState();
	bool bNeedsReset = false;
	FVector ResetGoal;
};
