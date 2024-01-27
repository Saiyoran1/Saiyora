#include "BehaviorTree/Decorators/BTD_TimeInCombatElapsed.h"

#include "BehaviorTree/BehaviorTree.h"
#include "GameFramework/GameStateBase.h"

UBTD_TimeInCombatElapsed::UBTD_TimeInCombatElapsed()
{
	NodeName = "Time in Combat Elapsed";
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bCreateNodeInstance = true;
	//There's a macro that works for these, but I don't understand it.
}

void UBTD_TimeInCombatElapsed::OnInstanceCreated(UBehaviorTreeComponent& OwnerComp)
{
	Super::OnInstanceCreated(OwnerComp);
	
	UE_LOG(LogTemp, Warning, TEXT("My stupid decorator being initialized with owning controller %s!"),
		*OwnerComp.GetOwner()->GetName());
	
	CombatStartTime = OwnerComp.GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	
	UE_LOG(LogTemp, Warning, TEXT("Set combat start time for %s as %f."), *OwnerComp.GetOwner()->GetName(), CombatStartTime);
	
	FTimerDelegate LockoutDelegate;
	LockoutDelegate.BindUFunction(this, FName("OnLockoutComplete"), TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp));
	OwnerComp.GetWorld()->GetTimerManager().SetTimer(LockoutHandle, LockoutDelegate, LockoutDuration, false);
}

void UBTD_TimeInCombatElapsed::OnInstanceDestroyed(UBehaviorTreeComponent& OwnerComp)
{
	Super::OnInstanceDestroyed(OwnerComp);
	UE_LOG(LogTemp, Warning, TEXT("Instance getting destroyed!"));
	OwnerComp.GetWorld()->GetTimerManager().ClearTimer(LockoutHandle);
}

void UBTD_TimeInCombatElapsed::OnLockoutComplete(TWeakObjectPtr<UBehaviorTreeComponent> OwnerComp)
{
	if (!IsValid(OwnerComp.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Lockout ended but owner comp wasn't valid!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Lockout ending at %f"), OwnerComp->GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
	bTempAborting = true;
	ConditionalFlowAbort(*OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
	bTempAborting = false;
}

bool UBTD_TimeInCombatElapsed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const float TimePassed = (OwnerComp.GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - CombatStartTime);
	UE_LOG(LogTemp, Warning, TEXT("Evaluating decorator. From abort? %s"), *FString(bTempAborting ? "Yup" : "Nope"));
	return TimePassed >= LockoutDuration;
}

