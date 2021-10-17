#include "RootMotionHandler.h"
#include "PlayerCombatAbility.h"
#include "GameFramework/CharacterMovementComponent.h"

void URootMotionHandler::Apply()
{
	//Override.
}

URootMotionJumpForceHandler* URootMotionJumpForceHandler::RootMotionJumpForce(UPlayerCombatAbility* Source,
                                                                              UCharacterMovementComponent* Movement, FRotator Rotation, float Distance, float Height, float Duration, float MinimumLandedTriggerTime,
                                                                              bool bFinishOnLanded, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish,
                                                                              float ClampVelocityOnFinish, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve)
{
	if (!IsValid(Movement) || !IsValid(Source) || Source->GetHandler()->GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}
	URootMotionJumpForceHandler* MyTask = NewObject<URootMotionJumpForceHandler>(Source->GetHandler()->GetOwner());
	MyTask->TargetMovement = Movement;
    MyTask->Rotation = Rotation;
    MyTask->Distance = Distance;
    MyTask->Height = Height;
    MyTask->Duration = Duration;
	MyTask->bHasDuration = Duration != 0.0f;
    MyTask->MinimumLandedTriggerTime = MinimumLandedTriggerTime * Duration;
    MyTask->bFinishOnLanded = bFinishOnLanded;
    MyTask->FinishVelocityMode = VelocityOnFinishMode;
    MyTask->FinishSetVelocity = SetVelocityOnFinish;
    MyTask->FinishClampVelocity = ClampVelocityOnFinish;
    MyTask->PathOffsetCurve = PathOffsetCurve;
    MyTask->TimeMappingCurve = TimeMappingCurve;
	MyTask->StartTime = Source->GetWorld()->GetTimeSeconds();
	MyTask->EndTime = MyTask->StartTime + Duration;
	MyTask->Apply();
}
