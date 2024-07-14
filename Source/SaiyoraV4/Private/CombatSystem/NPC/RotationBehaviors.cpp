#include "RotationBehaviors.h"
#include "AIController.h"
#include "TargetContexts.h"
#include "Navigation/PathFollowingComponent.h"

void FNPCRotationBehavior::Initialize(const AActor* Actor)
{
	InitializeBehavior(Actor);
	bInitialized = true;
}

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

void FNPCRB_SwitchOnDistance::InitializeBehavior(const AActor* Actor)
{
	FNPCRotationBehavior* InRange = InRangeBehavior.GetMutablePtr<FNPCRotationBehavior>();
	if (InRange)
	{
		InRange->Initialize(Actor);
	}
	FNPCRotationBehavior* OutRange = OutOfRangeBehavior.GetMutablePtr<FNPCRotationBehavior>();
	if (OutRange)
	{
		OutRange->Initialize(Actor);
	}
	//Set an enum value to decide whether we can actually switch between two valid behaviors, or if we should just use one of them, or if they're both invalid.
	SwitchBehavior = InRange ? OutRange ? ERotationSwitchBehavior::Valid : ERotationSwitchBehavior::InRange
		: OutRange ? ERotationSwitchBehavior::OutOfRange : ERotationSwitchBehavior::Invalid;
}

void FNPCRB_SwitchOnDistance::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const
{
	if (!IsValid(Actor))
	{
		return;
	}
	switch (SwitchBehavior)
	{
	case ERotationSwitchBehavior::Valid :
		{
			const FNPCTargetContext* Context = TargetContext.GetPtr<FNPCTargetContext>();
			if (!Context)
			{
				return;
			}
			const AActor* Target = Context->GetBestTarget(Actor);
			if (!IsValid(Target))
			{
				return;
			}
			const float Distance = bIncludeZDistance ? FVector::Dist(Target->GetActorLocation(), Actor->GetActorLocation()) : FVector::Dist2D(Target->GetActorLocation(), Actor->GetActorLocation());
			if (bPreviouslyInRange)
			{
				if (Distance > DistanceThreshold * (FMath::Max(0.0f, 1.0f + StickinessFactor)))
				{
					if (const FNPCRotationBehavior* OutRange = OutOfRangeBehavior.GetPtr<FNPCRotationBehavior>())
					{
						OutRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
					}
					bPreviouslyInRange = false;
				}
				else
				{
					if (const FNPCRotationBehavior* InRange = InRangeBehavior.GetPtr<FNPCRotationBehavior>())
					{
						InRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
					}
				}
			}
			else
			{
				if (Distance < DistanceThreshold * (FMath::Max(0.0f, 1.0f - StickinessFactor)))
				{
					if (const FNPCRotationBehavior* InRange = InRangeBehavior.GetPtr<FNPCRotationBehavior>())
					{
						InRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
					}
					bPreviouslyInRange = true;
				}
				else
				{
					if (const FNPCRotationBehavior* OutRange = OutOfRangeBehavior.GetPtr<FNPCRotationBehavior>())
					{
						OutRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
					}
				}
			}
		}
		break;
	case ERotationSwitchBehavior::InRange :
		{
			if (const FNPCRotationBehavior* InRange = InRangeBehavior.GetPtr<FNPCRotationBehavior>())
			{
				InRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
			}
		}
		break;
	case ERotationSwitchBehavior::OutOfRange :
		{
			if (const FNPCRotationBehavior* OutRange = OutOfRangeBehavior.GetPtr<FNPCRotationBehavior>())
			{
				OutRange->ModifyRotation(DeltaTime, Actor, OutRotation, UnclampedRotation);
			}
		}
		break;
	default :
		break;
	}
}