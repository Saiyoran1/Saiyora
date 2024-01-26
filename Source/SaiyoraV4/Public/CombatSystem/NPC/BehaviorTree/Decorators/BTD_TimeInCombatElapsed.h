#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_TimeInCombatElapsed.generated.h"

//Decorator that passes once a certain amount of time has passed since combat started.

UCLASS()
class SAIYORAV4_API UBTD_TimeInCombatElapsed : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTD_TimeInCombatElapsed();

protected:
	
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:

	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float LockoutDuration = 1.0f;

	float CombatStartTime = 0.0f;
	FTimerHandle LockoutHandle;
	UFUNCTION()
	void OnLockoutComplete(TWeakObjectPtr<UBehaviorTreeComponent> OwnerComp);

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

};
