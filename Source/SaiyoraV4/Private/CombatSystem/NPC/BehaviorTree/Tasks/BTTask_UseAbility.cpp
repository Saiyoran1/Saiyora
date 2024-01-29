#include "Tasks/BTTask_UseAbility.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "SaiyoraCombatInterface.h"

EBTNodeResult::Type UBTTask_UseAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (!OwnerComp.GetAIOwner()->GetPawn()->Implements<USaiyoraCombatInterface>())
	{
		return EBTNodeResult::Failed;
	}
	UAbilityComponent* OwnerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwnerComp.GetAIOwner()->GetPawn());
	if (!IsValid(OwnerAbilityComp))
	{
		return EBTNodeResult::Failed;
	}
	//Who knows what happens when we don't use an instant ability.
	const FAbilityEvent Event = OwnerAbilityComp->UseAbility(AbilityClass);
	return Event.ActionTaken == ECastAction::Fail ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
}
