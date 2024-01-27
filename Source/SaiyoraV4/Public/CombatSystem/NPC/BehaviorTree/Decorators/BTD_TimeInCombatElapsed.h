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

	void CallConditionalFlowAbort(UBehaviorTreeComponent* OwningComp) const;

protected:

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

private:

	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float LockoutDuration = 1.0f;

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
