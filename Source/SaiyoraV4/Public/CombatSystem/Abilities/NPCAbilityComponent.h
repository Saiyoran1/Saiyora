#pragma once
#include <NPCEnums.h>

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "DungeonGameState.h"
#include "NPCAbilityComponent.generated.h"

class UBehaviorTree;
class UThreatHandler;
class UCombatStatusComponent;
class USaiyoraMovementComponent;

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

	//Patrolling

	//Resetting

private:

	bool bNeedsReset = false;
	
};
