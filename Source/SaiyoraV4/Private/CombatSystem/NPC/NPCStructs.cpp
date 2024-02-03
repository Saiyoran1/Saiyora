#include "NPCStructs.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FNPCActionChoice::SetupConditions(AActor* Owner)
{
	for (FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		FNPCActionCondition* Condition = InstancedCondition.GetMutablePtr<FNPCActionCondition>();
		if (Condition)
		{
			Condition->SetupConditionChecks(Owner);
			if (bHighPriority)
			{
				Condition->OnConditionUpdated.BindRaw(this, &FNPCAbilityChoice::UpdateOnConditionMet);
			}
		}
	}
}

bool FNPCActionChoice::AreConditionsMet() const
{
	for (const FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		const FNPCActionCondition* Condition = InstancedCondition.GetPtr<FNPCActionCondition>();
		if (Condition && !Condition->IsConditionMet())
		{
			return false;
		}
	}
	return true;
}

void FNPCActionChoice::TickConditions(AActor* Owner, const float DeltaTime)
{
	for (FInstancedStruct& InstancedCondition : ChoiceConditions)
	{
		FNPCActionCondition* Condition = InstancedCondition.GetMutablePtr<FNPCActionCondition>();
		if (Condition && Condition->bRequiresTick)
		{
			Condition->TickCondition(Owner, DeltaTime);
		}
	}
}

void FNPCActionChoice::UpdateOnConditionMet()
{
	if (AreConditionsMet())
	{
		OnChoiceAvailable.Execute(ChoiceIndex);
	}
}

bool FNPCMovementWorldLocation::ExecuteMovementChoice(AAIController* AIController)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(WorldLocation);
	MoveRequest.SetAcceptanceRadius(Tolerance);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetCanStrafe(true);
	MoveRequest.SetUsePathfinding(true);
	
	const FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);
	return Result.Code == EPathFollowingResult::Success;
}