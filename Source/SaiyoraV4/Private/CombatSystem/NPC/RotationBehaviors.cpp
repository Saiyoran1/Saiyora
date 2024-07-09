#include "RotationBehaviors.h"

#include "AIController.h"
#include "TargetContexts.h"
#include "Navigation/PathFollowingComponent.h"

void FNPCRB_OrientToMovement::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const
{
	const FVector Velocity = bOnlyYaw ? Actor->GetVelocity().GetSafeNormal2D() : Actor->GetVelocity().GetSafeNormal();
	//If we have no velocity, don't adjust rotation at all.
	if (Velocity == FVector::ZeroVector)
	{
		return;
	}
	DrawDebugDirectionalArrow(Actor->GetWorld(), Actor->GetActorLocation(), Actor->GetActorLocation() + OutRotation.Vector() * 200.0f, 100.0f, FColor::Red);
	if (bEnforceRotationSpeed)
	{
		DrawDebugDirectionalArrow(Actor->GetWorld(), Actor->GetActorLocation(), Actor->GetActorLocation() + Velocity * 200.0f, 100.0f, FColor::Yellow);
		const FRotator FinalRotation = FMath::VInterpNormalRotationTo(OutRotation.Vector(), Velocity, DeltaTime, MaxRotationSpeed).Rotation();
		if (bOnlyYaw)
		{
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = Velocity.Rotation().Yaw;
		}
		else
		{
			OutRotation = Velocity.Rotation();
		}
	}
	DrawDebugDirectionalArrow(Actor->GetWorld(), Actor->GetActorLocation(), Actor->GetActorLocation() + OutRotation.Vector() * 200.0f, 100.0f, FColor::Green);
}

void FNPCRB_OrientToTarget::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const
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
	const FVector TowardTarget = bOnlyYaw ? (TargetActor->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal2D() : (TargetActor->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal();
	if (bEnforceRotationSpeed)
	{
		const FRotator FinalRotation = FMath::VInterpNormalRotationTo(OutRotation.Vector(), TowardTarget, DeltaTime, MaxRotationSpeed).Rotation();
		if (bOnlyYaw)
		{
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = (TargetActor->GetActorLocation() - Actor->GetActorLocation()).Rotation().Yaw;
		}
		else
		{
			OutRotation = (TargetActor->GetActorLocation() - Actor->GetActorLocation()).Rotation();
		}
	}
}

void FNPCRB_OrientToPath::ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const
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
	const FVector Direction = bOnlyYaw ? Controller->GetPathFollowingComponent()->GetCurrentDirection().GetSafeNormal2D() : Controller->GetPathFollowingComponent()->GetCurrentDirection().GetSafeNormal();
	if (bEnforceRotationSpeed)
	{
		const FRotator FinalRotation = FMath::VInterpNormalRotationTo(OutRotation.Vector(), Direction, DeltaTime, MaxRotationSpeed).Rotation();
		if (bOnlyYaw)
		{
			OutRotation.Yaw = FinalRotation.Yaw;
		}
		else
		{
			OutRotation = FinalRotation;
		}
	}
	else
	{
		if (bOnlyYaw)
		{
			OutRotation.Yaw = Direction.Rotation().Yaw;
		}
		else
		{
			OutRotation = Direction.Rotation();
		}
	}
}