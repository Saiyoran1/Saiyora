#include "BTD_AbilityUsable.h"

#include "AbilityComponent.h"
#include "AIController.h"
#include "SaiyoraCombatInterface.h"

UBTD_AbilityUsable::UBTD_AbilityUsable()
{
	NodeName = "Is Ability Usable";
	bNotifyTick = true;
}

void UBTD_AbilityUsable::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
	
	FBTDMemory_AbilityUsable* Memory = CastInstanceNodeMemory<FBTDMemory_AbilityUsable>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	if (!OwnerComp.GetAIOwner()->GetPawn()->Implements<USaiyoraCombatInterface>())
	{
		Memory->Ability = nullptr;
		Memory->bUsable = false;
		return;
	}
	const UAbilityComponent* AbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwnerComp.GetAIOwner()->GetPawn());
	if (!IsValid(AbilityComp))
	{
		Memory->Ability = nullptr;
		Memory->bUsable = false;
		return;
	}
	Memory->Ability = AbilityComp->FindActiveAbility(AbilityClass);
	if (!IsValid(Memory->Ability))
	{
		Memory->Ability = nullptr;
		Memory->bUsable = false;
		return;
	}
	TArray<ECastFailReason> FailReasons;
	Memory->bUsable = Memory->Ability->IsCastable(FailReasons);
}

void UBTD_AbilityUsable::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	Super::CleanupMemory(OwnerComp, NodeMemory, CleanupType);

	FBTDMemory_AbilityUsable* Memory = CastInstanceNodeMemory<FBTDMemory_AbilityUsable>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	Memory->Ability = nullptr;
	Memory->bUsable = false;
}

void UBTD_AbilityUsable::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	FBTDMemory_AbilityUsable* Memory = CastInstanceNodeMemory<FBTDMemory_AbilityUsable>(NodeMemory);
	if (!Memory)
	{
		return;
	}
	const bool bPreviouslyUsable = Memory->bUsable;
	TArray<ECastFailReason> FailReasons;
	Memory->bUsable = Memory->bUsable = IsValid(Memory->Ability) ? Memory->Ability->IsCastable(FailReasons) : false;
	if (Memory->bUsable != bPreviouslyUsable)
	{
		ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
	}
}

bool UBTD_AbilityUsable::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const FBTDMemory_AbilityUsable* Memory = CastInstanceNodeMemory<FBTDMemory_AbilityUsable>(NodeMemory);
	if (!Memory)
	{
		return false;
	}
	return Memory->bUsable;
}
