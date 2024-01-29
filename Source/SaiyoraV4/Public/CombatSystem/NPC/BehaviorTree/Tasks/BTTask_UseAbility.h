#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UseAbility.generated.h"

class UCombatAbility;

UCLASS()
class SAIYORAV4_API UBTTask_UseAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:

	//The ability class to try and use.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCombatAbility> AbilityClass;
};
