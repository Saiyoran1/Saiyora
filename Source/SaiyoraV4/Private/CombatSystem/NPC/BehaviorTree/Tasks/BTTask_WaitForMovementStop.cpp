#include "BTTask_WaitForMovementStop.h"
#include "AIController.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"

UBTTask_WaitForMovementStop::UBTTask_WaitForMovementStop()
{
	NodeName = "Wait for Movement Stop";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_WaitForMovementStop::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTTMemory_WaitForMovementStop* Memory = CastInstanceNodeMemory<FBTTMemory_WaitForMovementStop>(NodeMemory);
	if (!Memory)
	{
		return EBTNodeResult::Failed;
	}
	if (!IsValid(Memory->OwnerMovement))
	{
		return EBTNodeResult::Failed;
	}
	if (!Memory->OwnerMovement->IsMoving())
	{
		return EBTNodeResult::Succeeded;
	}
	Memory->TimeElapsed = 0.0f;
	return EBTNodeResult::InProgress;
}

void UBTTask_WaitForMovementStop::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTTMemory_WaitForMovementStop* Memory = CastInstanceNodeMemory<FBTTMemory_WaitForMovementStop>(NodeMemory);
	if (!Memory)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (!IsValid(Memory->OwnerMovement))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (!Memory->OwnerMovement->IsMoving())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	Memory->TimeElapsed += DeltaSeconds;
	if (Memory->TimeElapsed > TimeoutLength)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_WaitForMovementStop::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	FBTTMemory_WaitForMovementStop* Memory = CastInstanceNodeMemory<FBTTMemory_WaitForMovementStop>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	Memory->TimeElapsed = 0.0f;
	if (IsValid(OwnerComp.GetAIOwner()->GetPawn()) && OwnerComp.GetAIOwner()->GetPawn()->Implements<USaiyoraCombatInterface>())
	{
		Memory->OwnerMovement = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(OwnerComp.GetAIOwner()->GetPawn());
	}
}

void UBTTask_WaitForMovementStop::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	Super::CleanupMemory(OwnerComp, NodeMemory, CleanupType);

	FBTTMemory_WaitForMovementStop* Memory = CastInstanceNodeMemory<FBTTMemory_WaitForMovementStop>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	Memory->OwnerMovement = nullptr;
	Memory->TimeElapsed = 0.0f;
}
