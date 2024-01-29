#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_TimeInCombatElapsed.generated.h"

//Decorator that passes once a certain amount of time has passed since combat started.

struct FBTDMemory_TimeInCombatElapsed
{
	UBehaviorTreeComponent* OwningComp = nullptr;
	const class UBTD_TimeInCombatElapsed* OwningNode = nullptr;
	bool bLockoutFinished = false;
	FTimerHandle LockoutHandle;
	void OnTimerExpired();
};

UCLASS()
class SAIYORAV4_API UBTD_TimeInCombatElapsed : public UBTDecorator
{
	GENERATED_BODY()
	
	friend struct FBTDMemory_TimeInCombatElapsed;

public:

	UBTD_TimeInCombatElapsed();

protected:

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTDMemory_TimeInCombatElapsed); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

private:

	//How much time must pass since the tree started running before this decorator evaluates to true.
	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float LockoutDuration = 1.0f;
	
};
