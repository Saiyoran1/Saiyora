#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UseAbility.generated.h"

class UAbilityComponent;
class UCombatAbility;

UCLASS()
class SAIYORAV4_API UBTTask_UseAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_UseAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:

	//The ability class to try and use.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCombatAbility> AbilityClass;

	UPROPERTY()
	UAbilityComponent* OwnerAbilityComp = nullptr;
	UPROPERTY()
	UBehaviorTreeComponent* OwningComp = nullptr;

	UFUNCTION()
	void OnAbilityInterrupted(const FInterruptEvent& Event);
	UFUNCTION()
	void OnAbilityCancelled(const FCancelEvent& Event);
	UFUNCTION()
	void OnAbilityTicked(const FAbilityEvent& Event);
};
