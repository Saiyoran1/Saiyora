#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "DungeonGameState.h"
#include "NPCEnums.h"
#include "SaiyoraAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ActorComponent.h"
#include "Engine/TargetPoint.h"
#include "Navigation/PathFollowingComponent.h"
#include "NPCBehavior.generated.h"

class UDamageHandler;
class UThreatHandler;
class UStatHandler;
class USaiyoraMovementComponent;

USTRUCT(BlueprintType)
struct FPatrolPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ATargetPoint* Point = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WaitTime = 0.0f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UNPCBehavior : public UActorComponent
{
	GENERATED_BODY()

public:	

	UNPCBehavior();

protected:

	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

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

	void StopBehavior();
	void UpdateCombatStatus();

//Patrolling

public:
	
	UFUNCTION(BlueprintPure, Category = "Patrol")
	void GetPatrol(TArray<FPatrolPoint>& OutPatrol) const { OutPatrol = Patrol; }
	UFUNCTION(BlueprintPure, Category = "Patrol")
	bool ShouldPatrol() const { return Patrol.Num() > 1; }
	UFUNCTION(BlueprintPure, Category = "Patrol")
	bool ShouldLoopPatrol() const { return bLoopPatrol; }

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
	void StartPatrol();
	UFUNCTION()
	void OnPatrolComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result);
	
	
	//Combat

protected:

	UPROPERTY(EditAnywhere, Category = "Combat")
	UBehaviorTree* CombatTree = nullptr;

private:

	UFUNCTION()
	void OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnCombatChanged(const bool bInCombat);

	//Resetting

private:

	bool bNeedsReset = false;
	FVector ResetGoal;
	
};