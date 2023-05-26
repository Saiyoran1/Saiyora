#pragma once
#include <NPCEnums.h>
#include "CoreMinimal.h"
#include "AbilityChoice.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "DungeonGameState.h"
#include "Engine/TargetPoint.h"
#include "NPCAbilityComponent.generated.h"

class UBehaviorTree;
class UThreatHandler;
class UCombatStatusComponent;
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

USTRUCT(BlueprintType)
struct FCombatPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Phase"))
	FGameplayTag PhaseTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<TSubclassOf<UAbilityChoice>> AbilityChoices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHighPriority = false;

	bool operator==(const FCombatPhase& Other) const { return Other.PhaseTag.MatchesTagExact(PhaseTag); }
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UNPCAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

	UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer)

protected:

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

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
	UPROPERTY()
	USaiyoraMovementComponent* MovementComponentRef;

	ENPCCombatStatus CombatStatus = ENPCCombatStatus::None;
	void UpdateCombatStatus();
	UFUNCTION()
	void OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController) { AIController = Cast<AAIController>(NewController); UpdateCombatStatus(); }
	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase) { UpdateCombatStatus(); }
	UFUNCTION()
	void OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus) { UpdateCombatStatus(); }
	UFUNCTION()
	void OnCombatChanged(const bool bInCombat) { UpdateCombatStatus(); }

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
	TSet<FCombatPhase> Phases;
	FGameplayTag CurrentPhase;

	//Patrolling

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

	//Resetting

private:

	bool bNeedsReset = false;
	
};
