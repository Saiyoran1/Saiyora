#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_AbilityUsable.generated.h"

class UAbilityComponent;
class UCombatAbility;

struct FBTDMemory_AbilityUsable
{
	bool bUsable = false;
	UAbilityComponent* OwnerAbilityComp = nullptr;
};

UCLASS()
class SAIYORAV4_API UBTD_AbilityUsable : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTD_AbilityUsable();

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTDMemory_AbilityUsable); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

private:

	//The class of the ability that must be usable for this decorator to succeed.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCombatAbility> AbilityClass;
};
