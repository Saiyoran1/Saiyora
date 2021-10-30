#include "SaiyoraRootMotionHandler.h"
#include "PlayerCombatAbility.h"
#include "SaiyoraMovementComponent.h"
#include "UnrealNetwork.h"

void USaiyoraRootMotionHandler::OnMispredicted(int32 const PredictionID, ECastFailReason const FailReason)
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
	if (IsValid(TargetMovement))
	{
		if (SourcePredictionID != 0 && TargetMovement->GetOwnerRole() == ROLE_AutonomousProxy && IsValid(TargetMovement->GetOwnerAbilityHandler()))
		{
			TargetMovement->GetOwnerAbilityHandler()->UnsubscribeFromAbilityMispredicted(MispredictionCallback);
		}
		TargetMovement->RemoveRootMotionHandler(this);
	}
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
	bool const bStartedFinished = bFinished;
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
}

void USaiyoraRootMotionHandler::Init(USaiyoraMovementComponent* Movement, int32 const PredictionID, UObject* MoveSource)
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
	if (SourcePredictionID != 0 && TargetMovement->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		MispredictionCallback.BindDynamic(this, &USaiyoraRootMotionHandler::OnMispredicted);
	}
}

void USaiyoraRootMotionHandler::Apply()
{
	if (bFinished || !bInitialized)
	{
		return;
	}
	TSharedPtr<FRootMotionSource> const NewSource = MakeRootMotionSource();
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
		TargetMovement->GetOwnerAbilityHandler()->SubscribeToAbilityMispredicted(MispredictionCallback);
	}
	PostInit();
}

void UTestJumpForceHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTestJumpForceHandler, Rotation);
	DOREPLIFETIME(UTestJumpForceHandler, Distance);
	DOREPLIFETIME(UTestJumpForceHandler, Height);
	DOREPLIFETIME(UTestJumpForceHandler, Duration);
	DOREPLIFETIME(UTestJumpForceHandler, MinimumLandedTriggerTime);
	DOREPLIFETIME(UTestJumpForceHandler, bFinishOnLanded);
	DOREPLIFETIME(UTestJumpForceHandler, PathOffsetCurve);
	DOREPLIFETIME(UTestJumpForceHandler, TimeMappingCurve);
}

TSharedPtr<FRootMotionSource> UTestJumpForceHandler::MakeRootMotionSource()
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
	JumpForce->bDisableTimeout = false;
	JumpForce->PathOffsetCurve = PathOffsetCurve;
	JumpForce->TimeMappingCurve = TimeMappingCurve;
	JumpForce->AccumulateMode = AccumulateMode;
	JumpForce->Priority = Priority;
	JumpForce->FinishVelocityParams.Mode = FinishVelocityMode;
	JumpForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
	JumpForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
	return JumpForce;
}
