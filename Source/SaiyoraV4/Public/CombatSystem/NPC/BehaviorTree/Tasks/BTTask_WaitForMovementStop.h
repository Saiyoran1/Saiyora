#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_WaitForMovementStop.generated.h"

class USaiyoraMovementComponent;

struct FBTTMemory_WaitForMovementStop
{
	USaiyoraMovementComponent* OwnerMovement = nullptr;
	float TimeElapsed = 0.0f;
};

UCLASS()
class SAIYORAV4_API UBTTask_WaitForMovementStop : public UBTTaskNode
{
	GENERATED_BODY()

	UBTTask_WaitForMovementStop();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTTMemory_WaitForMovementStop); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

	//How long to wait for movement to stop before doing something else.
	UPROPERTY(EditAnywhere)
	float TimeoutLength = 5.0f;
};
