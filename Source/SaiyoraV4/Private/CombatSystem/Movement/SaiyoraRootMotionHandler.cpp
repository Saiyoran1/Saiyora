#include "SaiyoraRootMotionHandler.h"

#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraMovementComponent.h"
#include "UnrealNetwork.h"
#include "GameFramework/Character.h"

void USaiyoraRootMotionHandler::OnMispredicted(const int32 PredictionID)
{
	if (SourcePredictionID != 0 && PredictionID == SourcePredictionID)
	{
		Expire();
	}
}

void USaiyoraRootMotionHandler::OnRep_Finished()
{
	if (bInitialized && bFinished)
	{
		Expire();
	}
}

void USaiyoraRootMotionHandler::PreDestroyFromReplication()
{
	if (!bFinished)
	{
		Expire();
	}
	Super::PreDestroyFromReplication();
}

void USaiyoraRootMotionHandler::Expire()
{
	bFinished = true;
	GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
	if (IsValid(TargetMovement))
	{
		if (SourcePredictionID != 0 && TargetMovement->GetOwnerRole() == ROLE_AutonomousProxy && IsValid(TargetMovement->GetOwnerAbilityHandler()))
		{
			TargetMovement->GetOwnerAbilityHandler()->OnAbilityMispredicted.RemoveDynamic(this, &USaiyoraRootMotionHandler::OnMispredicted);
		}
		TargetMovement->RemoveRootMotionHandler(this);
	}
	PostExpire();
}

UWorld* USaiyoraRootMotionHandler::GetWorld() const
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return GetOuter()->GetWorld();
	}
	return nullptr;
}

void USaiyoraRootMotionHandler::PostNetReceive()
{
	Super::PostNetReceive();
	if (!IsValid(TargetMovement))
	{
		return;
	}
	if (bInitialized)
	{
		return;
	}
	//Cache the bFinished value to see if we need to expire immediately after applying.
	const bool bStartedFinished = bFinished;
	TargetMovement->AddRootMotionHandlerFromReplication(this);
	bInitialized = true;
	//Set finished to false here (we have already cached the value). This allows us to check if Expire is called during Apply.
	bFinished = false;
	Apply();
	//If we need to expire immediately due to having replicated bFinished as true, and Expire wasn't already called, call it here.
	if (bStartedFinished && !bFinished)
	{
		Expire();
	}
}

void USaiyoraRootMotionHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraRootMotionHandler, TargetMovement);
	DOREPLIFETIME(USaiyoraRootMotionHandler, SourcePredictionID);
	DOREPLIFETIME(USaiyoraRootMotionHandler, Source);
	DOREPLIFETIME(USaiyoraRootMotionHandler, Priority);
	DOREPLIFETIME(USaiyoraRootMotionHandler, AccumulateMode);
	DOREPLIFETIME(USaiyoraRootMotionHandler, FinishVelocityMode);
	DOREPLIFETIME(USaiyoraRootMotionHandler, FinishSetVelocity);
	DOREPLIFETIME(USaiyoraRootMotionHandler, FinishClampVelocity);
	DOREPLIFETIME(USaiyoraRootMotionHandler, bFinished);
	DOREPLIFETIME(USaiyoraRootMotionHandler, Duration);
	DOREPLIFETIME(USaiyoraRootMotionHandler, bIgnoreRestrictions);
}

void USaiyoraRootMotionHandler::Init(USaiyoraMovementComponent* Movement, const int32 PredictionID, UObject* MoveSource)
{
	if (!IsValid(Movement) || bInitialized)
	{
		return;
	}
	bInitialized = true;
	bFinished = false;
	TargetMovement = Movement;
	SourcePredictionID = PredictionID;
	Source = MoveSource;
}

void USaiyoraRootMotionHandler::Apply()
{
	if (bFinished || !bInitialized)
	{
		return;
	}
	const TSharedPtr<FRootMotionSource> NewSource = MakeRootMotionSource();
	if (!NewSource.IsValid())
	{
		return;
	}
	RootMotionSourceID = TargetMovement->ApplyRootMotionSource(NewSource);
	if (bFinished)
	{
		return;
	}
	if (SourcePredictionID != 0 && TargetMovement->GetOwnerRole() == ROLE_AutonomousProxy && IsValid(TargetMovement->GetOwnerAbilityHandler()))
	{
		TargetMovement->GetOwnerAbilityHandler()->OnAbilityMispredicted.AddDynamic(this, &USaiyoraRootMotionHandler::OnMispredicted);
	}
	if (NeedsExpireTimer())
	{
		GetWorld()->GetTimerManager().SetTimer(ExpireHandle, this, &USaiyoraRootMotionHandler::Expire, Duration);
	}
	PostApply();
}

void USaiyoraRootMotionHandler::CancelRootMotion()
{
	if (!bFinished)
	{
		Expire();
	}
}

void UJumpForceHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UJumpForceHandler, Rotation);
	DOREPLIFETIME(UJumpForceHandler, Distance);
	DOREPLIFETIME(UJumpForceHandler, Height);
	DOREPLIFETIME(UJumpForceHandler, MinimumLandedTriggerTime);
	DOREPLIFETIME(UJumpForceHandler, bFinishOnLanded);
	DOREPLIFETIME(UJumpForceHandler, PathOffsetCurve);
	DOREPLIFETIME(UJumpForceHandler, TimeMappingCurve);
}

TSharedPtr<FRootMotionSource> UJumpForceHandler::MakeRootMotionSource()
{
	TSharedPtr<FRootMotionSource_JumpForce> JumpForce = MakeShared<FRootMotionSource_JumpForce>(FRootMotionSource_JumpForce());
	if (!JumpForce.IsValid())
	{
		return nullptr;
	}
	JumpForce->Distance = Distance;
	JumpForce->Height = Height;
	JumpForce->Duration = Duration;
	JumpForce->Rotation = Rotation;
	JumpForce->bDisableTimeout = bFinishOnLanded;
	JumpForce->PathOffsetCurve = PathOffsetCurve;
	JumpForce->TimeMappingCurve = TimeMappingCurve;
	JumpForce->AccumulateMode = AccumulateMode;
	JumpForce->Priority = Priority;
	JumpForce->FinishVelocityParams.Mode = FinishVelocityMode;
	JumpForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
	JumpForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
	return JumpForce;
}

void UJumpForceHandler::PostApply()
{
	if (bFinishOnLanded && IsValid(TargetMovement))
	{
		TargetMovement->GetCharacterOwner()->LandedDelegate.AddDynamic(this, &UJumpForceHandler::OnLanded);
	}
}

void UJumpForceHandler::PostExpire()
{
	if (bFinishOnLanded && IsValid(TargetMovement))
	{
		TargetMovement->GetCharacterOwner()->LandedDelegate.RemoveDynamic(this, &UJumpForceHandler::OnLanded);
	}
}

bool UJumpForceHandler::NeedsExpireTimer() const
{
	return Duration > 0.0f && !bFinishOnLanded;
}

void UJumpForceHandler::OnLanded(const FHitResult& Result)
{
	Expire();
}

void UConstantForceHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UConstantForceHandler, Force);
	DOREPLIFETIME(UConstantForceHandler, StrengthOverTime);
}

TSharedPtr<FRootMotionSource> UConstantForceHandler::MakeRootMotionSource()
{
	TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>(FRootMotionSource_ConstantForce());
	if (!ConstantForce.IsValid())
	{
		return nullptr;
	}
	ConstantForce->Duration = Duration;
	ConstantForce->AccumulateMode = AccumulateMode;
	ConstantForce->Priority = Priority;
	ConstantForce->FinishVelocityParams.Mode = FinishVelocityMode;
	ConstantForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
	ConstantForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
	ConstantForce->Force = Force;
	ConstantForce->StrengthOverTime = StrengthOverTime;
	return ConstantForce;
}

//Base Root Motion Task

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
}

void USaiyoraRootMotionTask::Activate()
{
	Super::Activate();
	if (IsValid(MovementRef))
	{
		TSharedPtr<FRootMotionSource> RMSource = MakeRootMotionSource();
		RMSHandle = MovementRef->ApplyRootMotionSource(RMSource);
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

void USaiyoraConstantForce::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraConstantForce, Force);
	DOREPLIFETIME(USaiyoraConstantForce, StrengthOverTime);
}

TSharedPtr<FRootMotionSource> USaiyoraConstantForce::MakeRootMotionSource()
{
	TSharedPtr<FRootMotionSource_ConstantForce> RMSource = MakeShared<FRootMotionSource_ConstantForce>(FRootMotionSource_ConstantForce());
	if (RMSource.IsValid())
	{
		RMSource->Duration = Duration;
		RMSource->Priority = Priority;
		RMSource->AccumulateMode = AccumulateMode;
		RMSource->FinishVelocityParams.Mode = FinishVelocityMode;
		RMSource->FinishVelocityParams.SetVelocity = FinishSetVelocity;
		RMSource->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
		RMSource->Force = Force;
		RMSource->StrengthOverTime = StrengthOverTime;
	}
	return RMSource;
}
