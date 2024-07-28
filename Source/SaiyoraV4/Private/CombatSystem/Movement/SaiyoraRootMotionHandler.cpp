#include "SaiyoraRootMotionHandler.h"

#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraMovementComponent.h"
#include "UnrealNetwork.h"
#include "GameFramework/Character.h"

#pragma region Base Root Motion Task

void USaiyoraRootMotionTask::Init(USaiyoraMovementComponent* Movement, UObject* MoveSource)
{
	if (!IsValid(Movement))
	{
		return;
	}
	MovementRef = Movement;
	Source = MoveSource;
	if (IsValid(Source))
	{
		const UCombatAbility* SourceAsAbility = Cast<UCombatAbility>(Source);
		if (IsValid(SourceAsAbility))
		{
			PredictedTick = FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick());
			AbilityCompRef = SourceAsAbility->GetHandler();
			if (AbilityCompRef->GetOwnerRole() == ROLE_AutonomousProxy)
			{
				AbilityCompRef->OnAbilityMispredicted.AddDynamic(this, &USaiyoraRootMotionTask::OnMispredicted);
			}
			if (SourceAsAbility->GetHandler()->GetOwner() == MovementRef->GetOwner())
			{
				bExternal = false;
			}
		}
	}
	
}

void USaiyoraRootMotionTask::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraRootMotionTask, Source);
	DOREPLIFETIME(USaiyoraRootMotionTask, Priority);
	DOREPLIFETIME(USaiyoraRootMotionTask, AccumulateMode);
	DOREPLIFETIME(USaiyoraRootMotionTask, FinishVelocityMode);
	DOREPLIFETIME(USaiyoraRootMotionTask, FinishSetVelocity);
	DOREPLIFETIME(USaiyoraRootMotionTask, FinishClampVelocity);
	DOREPLIFETIME(USaiyoraRootMotionTask, Duration);
	DOREPLIFETIME(USaiyoraRootMotionTask, bIgnoreRestrictions);
	DOREPLIFETIME(USaiyoraRootMotionTask, ServerWaitID);
	DOREPLIFETIME(USaiyoraRootMotionTask, MovementRef);
	DOREPLIFETIME(USaiyoraRootMotionTask, bExternal);
}

void USaiyoraRootMotionTask::Activate()
{
	Super::Activate();
	if (IsValid(MovementRef))
	{
		RMSource = MakeRootMotionSource();
		MovementRef->FlushServerMoves();
		RMSHandle = MovementRef->ExecuteRootMotionTask(this);
	}
}

void USaiyoraRootMotionTask::OnDestroy(bool bInOwnerFinished)
{
	if (IsValid(MovementRef) && RMSHandle != (uint16)ERootMotionSourceID::Invalid)
	{
		MovementRef->RemoveRootMotionSourceByID(RMSHandle);
	}
	Super::OnDestroy(bInOwnerFinished);
}

void USaiyoraRootMotionTask::OnMispredicted(const int32 PredictionID)
{
	if (PredictionID == PredictedTick.PredictionID)
	{
		EndTask();
	}
}

void USaiyoraRootMotionTask::OnRep_ServerWaitID()
{
	if (ServerWaitID != 0 && IsValid(MovementRef))
	{
		MovementRef->ExecuteServerRootMotion(this);
	}
}

#pragma endregion
#pragma region Constant Force

void USaiyoraConstantForce::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraConstantForce, Force);
	DOREPLIFETIME(USaiyoraConstantForce, StrengthOverTime);
}

TSharedPtr<FRootMotionSource> USaiyoraConstantForce::MakeRootMotionSource()
{
	TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>(FRootMotionSource_ConstantForce());
	if (ConstantForce.IsValid())
	{
		ConstantForce->Duration = Duration;
		ConstantForce->Priority = Priority;
		ConstantForce->AccumulateMode = AccumulateMode;
		if (bIgnoreZAccumulate)
		{
			ConstantForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
		}
		ConstantForce->FinishVelocityParams.Mode = FinishVelocityMode;
		ConstantForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
		ConstantForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
		ConstantForce->Force = Force;
		ConstantForce->StrengthOverTime = StrengthOverTime;
	}
	return ConstantForce;
}

#pragma endregion 