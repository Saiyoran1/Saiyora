#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ActorComponent.h"
#include "Engine/TargetPoint.h"
#include "NPCBehavior.generated.h"

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

private:

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	UBehaviorTree* BehaviorTree = nullptr;

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

	//Combat

protected:

	UPROPERTY(EditAnywhere, Category = "Combat")
	UBehaviorTree* CombatTree = nullptr;
	
};
