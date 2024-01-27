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
	
	virtual void OnInstanceCreated(UBehaviorTreeComponent& OwnerComp) override;
	virtual void OnInstanceDestroyed(UBehaviorTreeComponent& OwnerComp) override;

private:

	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float LockoutDuration = 1.0f;

	bool bTempAborting = false;
	float CombatStartTime = 0.0f;
	FTimerHandle LockoutHandle;
	UFUNCTION()
	void OnLockoutComplete(TWeakObjectPtr<UBehaviorTreeComponent> OwnerComp);

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

};
