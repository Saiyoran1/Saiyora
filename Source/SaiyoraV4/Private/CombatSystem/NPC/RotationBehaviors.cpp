#include "RotationBehaviors.h"
#include "AIController.h"
#include "TargetContexts.h"
#include "Navigation/PathFollowingComponent.h"

void FNPCRotationBehavior::Initialize(AActor* Actor)
{
	InitializeBehavior(Actor);
	bInitialized = true;
}

#pragma region Orient to Movement

void FNPCRB_OrientToMovement::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const
{
	const FVector Velocity = bOnlyYaw ? Actor->GetVelocity().GetSafeNormal2D() : Actor->GetVelocity().GetSafeNormal();
	//If we have no velocity, don't adjust rotation at all.
	if (Velocity == FVector::ZeroVector)
	{
		return;
	}
	const FRotator GoalRotation = Velocity.Rotation();
	if (bEnforceRotationSpeed)
	{
		const FRotator FinalRotation = FMath::RInterpConstantTo(OutRotation, GoalRotation, DeltaTime, MaxRotationSpeed);
		if (bOnlyYaw)
		{
			UnclampedRotation.Yaw = GoalRotation.Yaw;
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			UnclampedRotation = GoalRotation;
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = GoalRotation.Yaw;
		}
		else
		{
			OutRotation = GoalRotation;
		}
	}
}

#pragma endregion
#pragma region Orient to Target

void FNPCRB_OrientToTarget::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const
{
	const FNPCTargetContext* Target = TargetContext.GetPtr<FNPCTargetContext>();
	//If invalid target context, don't adjust rotation.
	if (!Target)
	{
		return;
	}
	const AActor* TargetActor = Target->GetBestTarget(Actor);
	//If no valid target, don't adjust rotation.
	if (!IsValid(TargetActor))
	{
		return;
	}
	const FRotator TowardTarget = (TargetActor->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal().Rotation();
	if (bEnforceRotationSpeed)
	{
		const FRotator FinalRotation = FMath::RInterpConstantTo(OutRotation, TowardTarget, DeltaTime, MaxRotationSpeed);
		if (bOnlyYaw)
		{
			UnclampedRotation.Yaw = TowardTarget.Yaw;
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			UnclampedRotation = TowardTarget;
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = TowardTarget.Yaw;
		}
		else
		{
			OutRotation = TowardTarget;
		}
	}
}

#pragma endregion 
#pragma region Orient to Path

void FNPCRB_OrientToPath::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const
{
	const APawn* ActorAsPawn = Cast<APawn>(Actor);
	if (!IsValid(ActorAsPawn))
	{
		return;
	}
	const AAIController* Controller = Cast<AAIController>(ActorAsPawn->GetController());
	if (!IsValid(Controller))
	{
		return;
	}
	if (!IsValid(Controller->GetPathFollowingComponent()))
	{
		return;
	}
	const FRotator Direction = Controller->GetPathFollowingComponent()->GetCurrentDirection().GetSafeNormal().Rotation();
	if (bEnforceRotationSpeed)
	{
		const FRotator FinalRotation = FMath::RInterpConstantTo(OutRotation, Direction, DeltaTime, MaxRotationSpeed);
		if (bOnlyYaw)
		{
			UnclampedRotation.Yaw = Direction.Yaw;
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			UnclampedRotation = Direction;
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = Direction.Yaw;
		}
		else
		{
			OutRotation = Direction;
		}
	}
}

#pragma endregion 
#pragma region Switch on Conditions

void FNPCRB_SwitchOnConditions::InitializeBehavior(AActor* Actor)
{
	FNPCRotationBehavior* MetBehavior = ConditionsMetBehavior.GetMutablePtr<FNPCRotationBehavior>();
	if (MetBehavior)
	{
		MetBehavior->Initialize(Actor);
	}
	FNPCRotationBehavior* UnmetBehavior = ConditionsUnmetBehavior.GetMutablePtr<FNPCRotationBehavior>();
	if (UnmetBehavior)
	{
		UnmetBehavior->Initialize(Actor);
	}
	//Set an enum value to decide whether we can actually switch between two valid behaviors, or if we should just use one of them, or if they're both invalid.
	Validity = MetBehavior ? UnmetBehavior ? ERotationSwitchValidity::BothValid : ERotationSwitchValidity::ConditionsMetValid
		: UnmetBehavior ? ERotationSwitchValidity::ConditionsUnmetValid : ERotationSwitchValidity::BothInvalid;
	//If we're actually able to swap between two valid behaviors, initialize conditions for checking.
	if (Validity == ERotationSwitchValidity::BothValid)
	{
		for (FInstancedStruct& InstancedCondition : Conditions)
		{
			if (FNPCChoiceRequirement* Condition = InstancedCondition.GetMutablePtr<FNPCChoiceRequirement>())
			{
				Condition->Init(Actor);
			}
		}
	}
}

void FNPCRB_SwitchOnConditions::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const
{
	if (!IsValid(Actor))
	{
		return;
	}
	switch (Validity)
	{
	case ERotationSwitchValidity::BothValid :
		{
			//For All criteria, we start as true and set to false if any requirement isn't met.
			//For Any criteria, we start as false and set to true if any requirement is met.
			bool bConditionsMet = Criteria == EChoiceRequirementCriteria::All;
			for (const FInstancedStruct& InstancedCondition : Conditions)
			{
				if (const FNPCChoiceRequirement* Requirement = InstancedCondition.GetPtr<FNPCChoiceRequirement>())
				{
					const bool bConditionMet = Requirement->IsMet();
					if (bConditionMet != bConditionsMet)
					{
						bConditionsMet = !bConditionsMet;
						break;
					}
				}
			}
			//If conditions are met, we do the conditions met behavior.
			if (bConditionsMet)
			{
				if (const FNPCRotationBehavior* MetBehavior = ConditionsMetBehavior.GetPtr<FNPCRotationBehavior>())
				{
					MetBehavior->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
				}
			}
			//If conditions aren't met, we do the conditions unmet behavior.
			else
			{
				if (const FNPCRotationBehavior* UnmetBehavior = ConditionsUnmetBehavior.GetPtr<FNPCRotationBehavior>())
				{
					UnmetBehavior->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
				}
			}
		}
		break;
	case ERotationSwitchValidity::ConditionsMetValid :
		{
			if (const FNPCRotationBehavior* MetBehavior = ConditionsMetBehavior.GetPtr<FNPCRotationBehavior>())
			{
				MetBehavior->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
			}
		}
		break;
	case ERotationSwitchValidity::ConditionsUnmetValid :
		{
			if (const FNPCRotationBehavior* UnmetBehavior = ConditionsUnmetBehavior.GetPtr<FNPCRotationBehavior>())
			{
				UnmetBehavior->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
			}
		}
		break;
	default :
		break;
	}
}

#pragma endregion 