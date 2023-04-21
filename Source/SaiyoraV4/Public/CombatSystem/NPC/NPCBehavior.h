#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "DungeonGameState.h"
#include "NPCEnums.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ActorComponent.h"
#include "Engine/TargetPoint.h"
#include "Navigation/PathFollowingComponent.h"
#include "NPCBehavior.generated.h"

class UDamageHandler;
class UThreatHandler;
class UStatHandler;
class USaiyoraMovementComponent;
class UAbilityChoice;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 PhaseIndex = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<TSubclassOf<UAbilityChoice>> AbilityChoices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHighPriority = false;

	bool operator==(const FCombatPhase& Other) const { return Other.PhaseIndex == PhaseIndex; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatrolLocationNotification, const FVector&, Location);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UNPCBehavior : public UActorComponent
{
	GENERATED_BODY()

public:	

	UNPCBehavior(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

	UPROPERTY(EditAnywhere, Category = "Behavior")
	UBehaviorTree* BehaviorTree;

private:

	ENPCCombatStatus CombatStatus = ENPCCombatStatus::None;
	UPROPERTY()
	ADungeonGameState* GameStateRef;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef;
	UPROPERTY()
	UStatHandler* StatHandlerRef;
	UPROPERTY()
	USaiyoraMovementComponent* MovementComponentRef;
	UPROPERTY()
	APawn* OwnerAsPawn;
	UPROPERTY()
	AAIController* AIController;
	UFUNCTION()
	void OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController);
	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase);
	
	void UpdateCombatStatus();

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

	FCombatModifierHandle PatrolMoveSpeedModHandle;
	float DefaultMaxWalkSpeed = 0.0f;
	int32 NextPatrolIndex = 0;
	bool bFinishedPatrolling = false;
	void EnterPatrolState();
	
	//Combat

public:
	
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Combat")
	FCombatPhase GetCurrentPhase() const;

protected:

	UFUNCTION(BlueprintAuthorityOnly, Category = "Combat")
	void EnterPhase(const int32 PhaseIndex);

private:

	UFUNCTION(BlueprintImplementableEvent)
	void SetupPhaseTransitions();
	
	TSet<FCombatPhase> Phases;
	int32 CurrentPhaseIndex = 0;
	UPROPERTY()
	TArray<UAbilityChoice*> CurrentChoices;
	int32 NewPhaseIndex = -1;
	bool bReadyForPhaseChange = false;
	void InterruptCurrentAction();
	void StartNewAction();
	UFUNCTION()
	void OnChoiceScoreChanged(UAbilityChoice* Choice, const float NewScore);
	
	UFUNCTION()
	void OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnCombatChanged(const bool bInCombat);

	//Resetting

public:

	UFUNCTION(BlueprintCallable, Category = "Resetting")
	void MarkResetComplete();

private:

	bool bNeedsReset = false;
	FVector ResetGoal;
	
};