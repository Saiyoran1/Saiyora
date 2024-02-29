#pragma once
#include "CoreMinimal.h"
#include "NPCStructs.h"
#include "GameFramework/Actor.h"
#include "CombatLink.generated.h"

class UNPCAbilityComponent;
class UThreatHandler;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolSegmentCompleted);

//Actor that links enemies together, causing them all to enter combat if any of them do.
//Can also link patrol routes, so that all actors patrolling must reach their next point before any can move on.
UCLASS()
class SAIYORAV4_API ACombatLink : public AActor
{
	GENERATED_BODY()

#pragma region Core

public:
	
	ACombatLink();

protected:
	
	virtual void BeginPlay() override;

#pragma endregion
#pragma region Combat Link

private:

	UPROPERTY(EditAnywhere, Category = "Combat Link")
	TArray<AActor*> LinkedActors;
	
	bool bGroupInCombat = false;
	UPROPERTY()
	TMap<UThreatHandler*, bool> ThreatHandlers;
	UFUNCTION()
	void OnActorCombat(UThreatHandler* Handler, const bool bInCombat);
	
#pragma endregion 
#pragma region Group Patrol

public:

	FOnPatrolSegmentCompleted OnPatrolSegmentCompleted;

private:

	UPROPERTY(EditAnywhere, Category = "Combat Link")
	bool bLinkPatrolPaths = false;
	
	UPROPERTY()
	TMap<const AActor*, bool> CompletedPatrolSegment;
	void CheckPatrolSegmentComplete();
	UFUNCTION()
	void OnActorPatrolSegmentFinished(AActor* PatrollingActor, const FVector& Location);
	UFUNCTION()
	void OnActorPatrolStateChanged(AActor* PatrollingActor, const EPatrolSubstate PatrolSubstate);

#pragma endregion 
#pragma region Editor
#if WITH_EDITORONLY_DATA
	
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	UBillboardComponent* Billboard = nullptr;
	
#endif
#pragma endregion 
};
