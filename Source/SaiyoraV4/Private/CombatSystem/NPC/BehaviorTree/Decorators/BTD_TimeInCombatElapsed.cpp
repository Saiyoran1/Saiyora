#include "BehaviorTree/Decorators/BTD_TimeInCombatElapsed.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameFramework/GameStateBase.h"

struct FBTDMemory_TimeInCombatElapsed
{
	UBehaviorTreeComponent* OwningComp = nullptr;
	const UBTD_TimeInCombatElapsed* OwningNode = nullptr;
	float InitTime = 0.0f;
	FTimerHandle LockoutHandle;
	void OnTimerExpired()
	{
		UE_LOG(LogTemp, Warning, TEXT("This shit worked lmao."));
		OwningNode->CallConditionalFlowAbort(OwningComp);
	}
};

UBTD_TimeInCombatElapsed::UBTD_TimeInCombatElapsed()
{
	NodeName = "Time in Combat Elapsed";
}

void UBTD_TimeInCombatElapsed::CallConditionalFlowAbort(UBehaviorTreeComponent* OwningComp) const
{
	ConditionalFlowAbort(*OwningComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}

void UBTD_TimeInCombatElapsed::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
	FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	Memory->InitTime = OwnerComp.GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	Memory->OwningNode = this;
	Memory->OwningComp = &OwnerComp;
	const FTimerDelegate LockoutDelegate = FTimerDelegate::CreateRaw(Memory, &FBTDMemory_TimeInCombatElapsed::OnTimerExpired);
	OwnerComp.GetWorld()->GetTimerManager().SetTimer(Memory->LockoutHandle, LockoutDelegate, LockoutDuration, false);
}

uint16 UBTD_TimeInCombatElapsed::GetInstanceMemorySize() const
{
	return sizeof(FBTDMemory_TimeInCombatElapsed);
}

void UBTD_TimeInCombatElapsed::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	Super::CleanupMemory(OwnerComp, NodeMemory, CleanupType);
	FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	if (Memory->OwningComp)
	{
		Memory->OwningComp->GetWorld()->GetTimerManager().ClearTimer(Memory->LockoutHandle);
	}
	//TODO: What does "cleaning up memory" even mean?
}

bool UBTD_TimeInCombatElapsed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	FBTDMemory_TimeInCombatElapsed* Memory = CastInstanceNodeMemory<FBTDMemory_TimeInCombatElapsed>(NodeMemory);
	UE_LOG(LogTemp, Warning, TEXT("Checking condition"));
	return Memory->OwningComp->GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - Memory->InitTime >= LockoutDuration;
}

