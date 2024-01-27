#include "BehaviorTree/Decorators/BTD_TimeInCombatElapsed.h"
#include "BehaviorTree/BehaviorTree.h"

void FBTDMemory_TimeInCombatElapsed::OnTimerExpired()
{
	bLockoutFinished = true;
	const bool bNodeValid = IsValid(OwningNode);
	const bool bTreeValid = IsValid(OwningComp);
	if (!bNodeValid || !bTreeValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat time elapsed decorator failed to abort because of invalid ref: %s"),
			*FString(bNodeValid ? bTreeValid ? "Both valid" : "Tree invalid" : bTreeValid ? "Node invalid" : "Both invalid"));
		return;
	}
	if (OwningComp->IsAuxNodeActive(OwningNode))
	{
		OwningNode->ConditionalFlowAbort(*OwningComp, EBTDecoratorAbortRequest::ConditionPassing);
	}
}

UBTD_TimeInCombatElapsed::UBTD_TimeInCombatElapsed()
{
	NodeName = "Time in Combat Elapsed";
}

void UBTD_TimeInCombatElapsed::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
	
	FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	Memory->OwningNode = this;
	Memory->OwningComp = &OwnerComp;
	Memory->bLockoutFinished = false;
	const FTimerDelegate LockoutDelegate = FTimerDelegate::CreateRaw(Memory, &FBTDMemory_TimeInCombatElapsed::OnTimerExpired);
	OwnerComp.GetWorld()->GetTimerManager().SetTimer(Memory->LockoutHandle, LockoutDelegate, LockoutDuration, false);
}

void UBTD_TimeInCombatElapsed::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	Super::CleanupMemory(OwnerComp, NodeMemory, CleanupType);
	
	FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	if (IsValid(Memory->OwningComp))
	{
		Memory->OwningComp->GetWorld()->GetTimerManager().ClearTimer(Memory->LockoutHandle);
	}
	Memory->OwningComp = nullptr;
	Memory->OwningNode = nullptr;
	Memory->bLockoutFinished = false;
}

bool UBTD_TimeInCombatElapsed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	return Memory->bLockoutFinished;
}

