#include "Tasks/BTTask_UseAbility.h"
#include "AbilityComponent.h"
#include "AIController.h"
#include "SaiyoraCombatInterface.h"

UBTTask_UseAbility::UBTTask_UseAbility()
{
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_UseAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (!OwnerComp.GetAIOwner()->GetPawn()->Implements<USaiyoraCombatInterface>())
	{
		return EBTNodeResult::Failed;
	}
	OwnerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwnerComp.GetAIOwner()->GetPawn());
	if (!IsValid(OwnerAbilityComp))
	{
		return EBTNodeResult::Failed;
	}
	const FAbilityEvent Event = OwnerAbilityComp->UseAbility(AbilityClass);
	if (Event.ActionTaken == ECastAction::Fail)
	{
		return EBTNodeResult::Failed;
	}
	//If we started a cast (didn't fail and are now casting), we have to wait to finish the cast to know whether this succeeded.
	if (OwnerAbilityComp->IsCasting())
	{
		OwningComp = &OwnerComp;
		OwnerAbilityComp->OnAbilityInterrupted.AddDynamic(this, &UBTTask_UseAbility::OnAbilityInterrupted);
		OwnerAbilityComp->OnAbilityCancelled.AddDynamic(this, &UBTTask_UseAbility::OnAbilityCancelled);
		OwnerAbilityComp->OnAbilityTick.AddDynamic(this, &UBTTask_UseAbility::OnAbilityTicked);
		return EBTNodeResult::InProgress;
	}
	return EBTNodeResult::Succeeded;
}

void UBTTask_UseAbility::OnAbilityInterrupted(const FInterruptEvent& Event)
{
	if (IsValid(OwningComp) && Event.bSuccess)
	{
		OwnerAbilityComp->OnAbilityInterrupted.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityInterrupted);
		OwnerAbilityComp->OnAbilityCancelled.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityCancelled);
		OwnerAbilityComp->OnAbilityTick.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityTicked);
		FinishLatentTask(*OwningComp, EBTNodeResult::Failed);
	}
}

void UBTTask_UseAbility::OnAbilityCancelled(const FCancelEvent& Event)
{
	if (IsValid(OwningComp) && Event.bSuccess)
	{
		OwnerAbilityComp->OnAbilityInterrupted.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityInterrupted);
		OwnerAbilityComp->OnAbilityCancelled.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityCancelled);
		OwnerAbilityComp->OnAbilityTick.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityTicked);
		FinishLatentTask(*OwningComp, EBTNodeResult::Failed);
	}
}

void UBTTask_UseAbility::OnAbilityTicked(const FAbilityEvent& Event)
{
	if (IsValid(OwningComp) && Event.ActionTaken == ECastAction::Complete)
	{
		OwnerAbilityComp->OnAbilityInterrupted.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityInterrupted);
		OwnerAbilityComp->OnAbilityCancelled.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityCancelled);
		OwnerAbilityComp->OnAbilityTick.RemoveDynamic(this, &UBTTask_UseAbility::OnAbilityTicked);
		FinishLatentTask(*OwningComp, EBTNodeResult::Succeeded);
	}
}

