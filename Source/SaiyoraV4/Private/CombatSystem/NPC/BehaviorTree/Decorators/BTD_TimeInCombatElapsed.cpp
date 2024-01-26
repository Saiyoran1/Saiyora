#include "BehaviorTree/Decorators/BTD_TimeInCombatElapsed.h"

#include "GameFramework/GameStateBase.h"

UBTD_TimeInCombatElapsed::UBTD_TimeInCombatElapsed()
{
	NodeName = "Time in Combat Elapsed";
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bCreateNodeInstance = true;
	//There's a macro that works for these, but I don't understand it.
}

void UBTD_TimeInCombatElapsed::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("My stupid decorator became relevant!"));
	CombatStartTime = OwnerComp.GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	UE_LOG(LogTemp, Warning, TEXT("Set combat start time for %s as %f."), *OwnerComp.GetOwner()->GetName(), CombatStartTime);
	FTimerDelegate LockoutDelegate;
	LockoutDelegate.BindUFunction(this, FName("OnLockoutComplete"), TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp));
	OwnerComp.GetWorld()->GetTimerManager().SetTimer(LockoutHandle, LockoutDelegate, LockoutDuration, false);
}

void UBTD_TimeInCombatElapsed::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("My stupid decorator became not relevant!"));
	OwnerComp.GetWorld()->GetTimerManager().ClearTimer(LockoutHandle);
}

void UBTD_TimeInCombatElapsed::OnLockoutComplete(TWeakObjectPtr<UBehaviorTreeComponent> OwnerComp)
{
	if (!OwnerComp.IsValid())
	{
		return;
	}
	ConditionalFlowAbort(*OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}

bool UBTD_TimeInCombatElapsed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const float TimePassed = (OwnerComp.GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - CombatStartTime);
	return TimePassed >= LockoutDuration;
}

