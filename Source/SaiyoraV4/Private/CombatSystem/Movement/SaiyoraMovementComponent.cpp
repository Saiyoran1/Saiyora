// Fill out your copyright notice in the Description page of Project Settings.

#include "Movement/SaiyoraMovementComponent.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/Character.h"
#include "Movement/MovementStructs.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"

bool USaiyoraMovementComponent::FSavedMove_Saiyora::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_Saiyora* CastMove = (FSavedMove_Saiyora*)&NewMove;
	if (CastMove)
	{
		if (bSavedWantsCustomMove != CastMove->bSavedWantsCustomMove || SavedPendingCustomMove.MoveType != CastMove->SavedPendingCustomMove.MoveType)
		{
			return false;
		}
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::Clear()
{
	Super::Clear();
	bSavedWantsCustomMove = false;
	SavedPendingCustomMove.Clear();
}

uint8 USaiyoraMovementComponent::FSavedMove_Saiyora::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsCustomMove)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	//Used for replaying on client. Copy data from an unacked saved move into the CMC.
	USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement());
	if (Movement)
	{
		Movement->PendingCustomMove = SavedPendingCustomMove;
	}
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	//Used for setting up the move before it is saved off. Sets the SavedMove params from the CMC.
	USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement());
	if (Movement)
	{
		bSavedWantsCustomMove = Movement->bWantsCustomMove;
		SavedPendingCustomMove = Movement->PendingCustomMove;
	}
}

USaiyoraMovementComponent::FNetworkPredictionData_Client_Saiyora::FNetworkPredictionData_Client_Saiyora(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
	//I guess this just stays empty???
}

FSavedMovePtr USaiyoraMovementComponent::FNetworkPredictionData_Client_Saiyora::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Saiyora());
}

void USaiyoraMovementComponent::FSaiyoraNetworkMoveData::ClientFillNetworkMoveData(
	const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	//Sets up the Network Move Data for sending to the server or replaying. Copies Saved Move data into the Network Move Data.
	FSavedMove_Saiyora const* CastMove = (FSavedMove_Saiyora*)&ClientMove;
	if (CastMove)
	{
		CustomMoveAbilityRequest.AbilityClass = CastMove->SavedPendingCustomMove.AbilityClass;
		CustomMoveAbilityRequest.PredictionID = CastMove->SavedPendingCustomMove.PredictionID;
		CustomMoveAbilityRequest.Tick = 0;
		CustomMoveAbilityRequest.PredictionParams = CastMove->SavedPendingCustomMove.PredictionParams;
		CustomMoveAbilityRequest.ClientStartTime = CastMove->SavedPendingCustomMove.OriginalTimestamp;
	}
}

bool USaiyoraMovementComponent::FSaiyoraNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement,
	FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	Ar << CustomMoveAbilityRequest;
	return !Ar.IsError();
}

USaiyoraMovementComponent::FSaiyoraNetworkMoveDataContainer::FSaiyoraNetworkMoveDataContainer() : Super()
{
	NewMoveData = &CustomDefaultMoveData[0];
	PendingMoveData = &CustomDefaultMoveData[1];
	OldMoveData = &CustomDefaultMoveData[2];
}

USaiyoraMovementComponent::USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetNetworkMoveDataContainer(CustomNetworkMoveDataContainer);
}

void USaiyoraMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	//Updates the CMC when replaying a client move or receiving the move on the server. Only affects flags.
	bWantsCustomMove = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* USaiyoraMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	if (!ClientPredictionData)
	{
		USaiyoraMovementComponent* MutableThis = const_cast<USaiyoraMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Saiyora(*this);
	}
	return ClientPredictionData;
}

void USaiyoraMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	if (!CharacterOwner)
	{
		return;
	}
	if (bWantsCustomMove)
	{
		ExecuteCustomMove();
		bWantsCustomMove = false;
		CustomMoveAbilityRequest.Clear();
		PendingCustomMove.Clear();
	}
}

void USaiyoraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameStateRef))
	{
		UE_LOG(LogTemp, Warning, (TEXT("Custom CMC encountered wrong Game State Ref!")));
		return;
	}
	OwnerAbilityHandler = Cast<UPlayerAbilityHandler>(ISaiyoraCombatInterface::Execute_GetAbilityHandler(GetOwner()));
	if (GetOwnerRole() == ROLE_AutonomousProxy && IsValid(OwnerAbilityHandler))
	{
		OnPredictedAbility.BindDynamic(this, &USaiyoraMovementComponent::OnCustomMoveCastPredicted);
		//Do not bind OnPredictedAbility, this will be bound and unbound only when custom moves are predicted.
		OnMispredict.BindDynamic(this, &USaiyoraMovementComponent::AbilityMispredicted);
		OwnerAbilityHandler->SubscribeToAbilityMispredicted(OnMispredict);
	}
}

void USaiyoraMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	const FVector& NewAccel)
{
	//Unpacks the Network Move Data for the CMC to use on the server or during replay. Copies Network Move Data into CMC.
	FSaiyoraNetworkMoveData* MoveData = static_cast<FSaiyoraNetworkMoveData*>(GetCurrentNetworkMoveData());
	if (MoveData)
	{
		CustomMoveAbilityRequest = MoveData->CustomMoveAbilityRequest;
	}
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void USaiyoraMovementComponent::ExecuteCustomMove()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && PawnOwner->IsLocallyControlled())
	{
		//If we are a listen server, bypass the need for flags. The movement itself is just manually called by ability.
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
	{
		//If we are the server, and have an auto proxy, check that this move hasn't already been sent (duplicating cast IDs), and can actually be performed.
		if (ServerCompletedMovementIDs.Contains(CustomMoveAbilityRequest.PredictionID) || !IsValid(OwnerAbilityHandler))
		{
			return;
		}
		bool const bCompleted = OwnerAbilityHandler->UseAbilityFromPredictedMovement(CustomMoveAbilityRequest);
		if (bCompleted)
		{
			//Document that this prediction ID has already been used, so duplicate moves with this ID do not get re-used.
			ServerCompletedMovementIDs.Add(CustomMoveAbilityRequest.PredictionID);
		}
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//Check to see if the pending move has been marked as a misprediction, to prevent it from being replayed.
		if (!CompletedCastStatus.FindRef(PendingCustomMove.PredictionID))
		{
			return;
		}
		switch (PendingCustomMove.MoveType)
		{
			case ESaiyoraCustomMove::None :
				break;
			case ESaiyoraCustomMove::TeleportDirection :
				ExecuteTeleportInDirection(PendingCustomMove.MoveParams.Direction, PendingCustomMove.MoveParams.Scale, PendingCustomMove.MoveParams.bSweep, PendingCustomMove.MoveParams.bIgnoreZ);
				break;
			case ESaiyoraCustomMove::TeleportLocation :
				ExecuteTeleportToLocation(PendingCustomMove.MoveParams.Target, PendingCustomMove.MoveParams.Rotation, PendingCustomMove.MoveParams.bSweep);
				break;
			case ESaiyoraCustomMove::Launch :
				ExecuteLaunchPlayer(PendingCustomMove.MoveParams.Direction, PendingCustomMove.MoveParams.Scale);
				break;
			default :
				break;
		}
	}
}

void USaiyoraMovementComponent::AbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason)
{
	CompletedCastStatus.Add(PredictionID, false);
}

void USaiyoraMovementComponent::SetupCustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType, FCustomMoveParams const& Params)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !IsValid(Source) || !IsValid(OwnerAbilityHandler))
	{
		return;
	}
	OwnerAbilityHandler->SubscribeToAbilityTicked(OnPredictedAbility);
	PendingCustomMove.MoveType = MoveType;
	PendingCustomMove.AbilityClass = Source->GetClass();
	PendingCustomMove.MoveParams = Params;
	PendingCustomMove.PredictionID = OwnerAbilityHandler->GetLastPredictionID();
	PendingCustomMove.OriginalTimestamp = GameStateRef->GetServerWorldTimeSeconds();
}

void USaiyoraMovementComponent::OnCustomMoveCastPredicted(FCastEvent const& Event)
{
	OwnerAbilityHandler->UnsubscribeFromAbilityTicked(OnPredictedAbility);
	CompletedCastStatus.Add(Event.PredictionID, true);
	if (!IsValid(PendingCustomMove.AbilityClass) || PendingCustomMove.AbilityClass != Event.Ability->GetClass()
		|| PendingCustomMove.PredictionID != Event.PredictionID || PendingCustomMove.MoveType == ESaiyoraCustomMove::None
			|| Event.Tick != 0)
	{
		PendingCustomMove.Clear();
		return;
	}
	PendingCustomMove.PredictionParams = Event.PredictionParams;
	bWantsCustomMove = true;
}

void USaiyoraMovementComponent::PredictTeleportInDirection(UPlayerCombatAbility* Source, FVector const& Direction, float const Length, bool const bSweep, bool const bIgnoreZ)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !IsValid(Source))
	{
		return;
	}
	FCustomMoveParams TeleParams;
	TeleParams.Direction = Direction;
	TeleParams.Scale = Length;
	TeleParams.bSweep = bSweep;
	TeleParams.bIgnoreZ = bIgnoreZ;
	SetupCustomMovement(Source, ESaiyoraCustomMove::TeleportDirection, TeleParams);
}

void USaiyoraMovementComponent::TeleportInDirection(UPlayerCombatAbility* Source, FVector const& Direction,
	float const Length, bool const bSweep, bool const bIgnoreZ)
{
	/*if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	ExecuteTeleportInDirection(Direction, Length, bSweep, bIgnoreZ);*/
	if (!IsValid(Source) || GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (!PawnOwner->IsLocallyControlled() && (OwnerAbilityHandler->GetLastPredictionID() == 0 || ServerCompletedMovementIDs.Contains(OwnerAbilityHandler->GetLastPredictionID())))
		{
			return;
		}
		ExecuteTeleportInDirection(Direction, Length, bSweep, bIgnoreZ);
		if (!PawnOwner->IsLocallyControlled())
		{
			ServerCompletedMovementIDs.Add(OwnerAbilityHandler->GetLastPredictionID());
		}
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		PredictTeleportInDirection(Source, Direction, Length, bSweep, bIgnoreZ);
	}
}

void USaiyoraMovementComponent::ExecuteTeleportInDirection(FVector const& Direction, float const Length, bool const bSweep, bool const bIgnoreZ)
{
	FVector Travel = Direction;
	if (bIgnoreZ)
	{
		Travel.Z = 0.0f;
	}
	Travel = Travel.GetSafeNormal();
	GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + (Length * Travel), bSweep);
	//TODO: Figure out how to make it so remote clients don't auto smooth small teleports?
}

void USaiyoraMovementComponent::PredictTeleportToLocation(UPlayerCombatAbility* Source, FVector const& Target,
	FRotator const& DesiredRotation, bool const bSweep)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !IsValid(Source))
	{
		return;
	}
	FCustomMoveParams TeleParams;
	TeleParams.Target = Target;
	TeleParams.Rotation = DesiredRotation;
	TeleParams.bSweep = bSweep;
	SetupCustomMovement(Source, ESaiyoraCustomMove::TeleportLocation, TeleParams);
}

void USaiyoraMovementComponent::TeleportToLocation(UPlayerCombatAbility* Source, FVector const& Target, FRotator const& DesiredRotation, bool const bSweep)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	ExecuteTeleportToLocation(Target, DesiredRotation, bSweep);
}

void USaiyoraMovementComponent::ExecuteTeleportToLocation(FVector const& Target, FRotator const& DesiredRotation, bool const bSweep)
{
	GetOwner()->SetActorLocation(Target, bSweep);
	GetOwner()->SetActorRotation(DesiredRotation);
}

void USaiyoraMovementComponent::PredictLaunchPlayer(UPlayerCombatAbility* Source, FVector const& Direction,
	float const Force)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !IsValid(Source))
	{
		return;
	}
	FCustomMoveParams LaunchParams;
	LaunchParams.Direction = Direction;
	LaunchParams.Scale = Force;
	SetupCustomMovement(Source, ESaiyoraCustomMove::Launch, LaunchParams);
}

void USaiyoraMovementComponent::LaunchPlayer(UPlayerCombatAbility* Source, FVector const& Direction, float const Force)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	ExecuteLaunchPlayer(Direction, Force);
}

void USaiyoraMovementComponent::ExecuteLaunchPlayer(FVector const& Direction, float const Force)
{
	Launch(Direction.GetSafeNormal() * Force);
}

/*
 Root Motion Testing
 */

void USaiyoraMovementComponent::TestRootMotion(UPlayerCombatAbility* Source)
{
	if (!IsValid(Source))
	{
		return;
	}
	TSharedPtr<FCustomJumpForce> CustomRootMotion = MakeShared<FCustomJumpForce>();
	CustomRootMotion->PredictionID = OwnerAbilityHandler->GetLastPredictionID();
	CustomRootMotion->Distance = 500.f;
	CustomRootMotion->Height = 500.f;
	CustomRootMotion->Duration = 0.5f;
	uint16 ID = ApplyRootMotionSource(CustomRootMotion);
	//TODO: Handle cleanup of source.
	URootMotionHandler* NewHandler = NewObject<URootMotionHandler>(GetOwner());
	if (IsValid(NewHandler))
	{
		NewHandler->Init(this, Cast<UPlayerAbilityHandler>(Source->GetHandler()), ID, true, 0.5f, CustomRootMotion->PredictionID);
		ActiveRootMotionHandlers.Add(NewHandler);
	}
}

void USaiyoraMovementComponent::ExpireHandledRootMotion(URootMotionHandler* Handler)
{
	if (IsValid(Handler))
	{
		if (ActiveRootMotionHandlers.Remove(Handler) > 0)
		{
			RemoveRootMotionSourceByID(Handler->GetHandledID());
		}
	}
}

void USaiyoraMovementComponent::JumpForce(UPlayerCombatAbility* Source, FRotator Rotation, float Distance, float Height,
	float Duration, float MinimumLandedTriggerTime, bool bFinishOnLanded,
	ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish,
	UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve)
{
	//TODO: Handling for creating, applying, and removing the jump force.
}

FCustomRootMotionSource::FCustomRootMotionSource()
{
	PredictionID = 0;
}

FRootMotionSource* FCustomRootMotionSource::Clone() const
{
	FCustomRootMotionSource* CopyPtr = new FCustomRootMotionSource(*this);
    return CopyPtr;
}

bool FCustomRootMotionSource::Matches(const FRootMotionSource* Other) const
{
	if (!Super::Matches(Other))
	{
		return false;
	}
	FCustomRootMotionSource const* CastOther = static_cast<FCustomRootMotionSource const*>(Other);
	if (CastOther)
	{
		return PredictionID == CastOther->PredictionID;
	}
	return false;
}

bool FCustomRootMotionSource::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!Super::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}
	Ar << PredictionID;
	bOutSuccess = true;
	return true;
}

UScriptStruct* FCustomRootMotionSource::GetScriptStruct() const
{
	return FCustomRootMotionSource::StaticStruct();
}

//Jump Force

FCustomJumpForce::FCustomJumpForce()
	: Rotation(ForceInitToZero)
	, Distance(-1.0f)
	, Height(-1.0f)
	, bDisableTimeout(false)
	, PathOffsetCurve(nullptr)
	, TimeMappingCurve(nullptr)
	, SavedHalfwayLocation(FVector::ZeroVector)
{
	// Don't allow partial end ticks. Jump forces are meant to provide velocity that
	// carries through to the end of the jump, and if we do partial ticks at the very end,
	// it means the provided velocity can be significantly reduced on the very last tick,
	// resulting in lost momentum. This is not desirable for jumps.
	Settings.SetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
}

bool FCustomJumpForce::IsTimeOutEnabled() const
{
	if (bDisableTimeout)
	{
		return false;
	}
	return FRootMotionSource::IsTimeOutEnabled();
}

FRootMotionSource* FCustomJumpForce::Clone() const
{
	FCustomJumpForce* CopyPtr = new FCustomJumpForce(*this);
	return CopyPtr;
}

bool FCustomJumpForce::Matches(const FRootMotionSource* Other) const
{
	if (!Super::Matches(Other))
	{
		return false;
	}

	// We can cast safely here since in FRootMotionSource::Matches() we ensured ScriptStruct equality
	const FCustomJumpForce* OtherCast = static_cast<const FCustomJumpForce*>(Other);

	return bDisableTimeout == OtherCast->bDisableTimeout &&
		PathOffsetCurve == OtherCast->PathOffsetCurve &&
		TimeMappingCurve == OtherCast->TimeMappingCurve &&
		FMath::IsNearlyEqual(Distance, OtherCast->Distance, SMALL_NUMBER) &&
		FMath::IsNearlyEqual(Height, OtherCast->Height, SMALL_NUMBER) &&
		Rotation.Equals(OtherCast->Rotation, 1.0f);
}

FVector FCustomJumpForce::GetPathOffset(float MoveFraction) const
{
	FVector PathOffset(FVector::ZeroVector);
	if (PathOffsetCurve)
	{
		// Calculate path offset
		PathOffset = PathOffsetCurve->GetVectorValue(MoveFraction);
	}
	else
	{
		// Default to "jump parabola", a simple x^2 shifted to be upside-down and shifted
		// to get [0,1] X (MoveFraction/Distance) mapping to [0,1] Y (height)
		// Height = -(2x-1)^2 + 1
		const float Phi = 2.f*MoveFraction - 1;
		const float Z = -(Phi*Phi) + 1;
		PathOffset.Z = Z;
	}

	// Scale Z offset to height. If Height < 0, we use direct path offset values
	if (Height >= 0.f)
	{
		PathOffset.Z *= Height;
	}

	return PathOffset;
}

FVector FCustomJumpForce::GetRelativeLocation(float MoveFraction) const
{
	// Given MoveFraction, what relative location should a character be at?
	FRotator FacingRotation(Rotation);
	FacingRotation.Pitch = 0.f; // By default we don't include pitch, but an option could be added if necessary

	FVector RelativeLocationFacingSpace = FVector(MoveFraction * Distance, 0.f, 0.f) + GetPathOffset(MoveFraction);

	return FacingRotation.RotateVector(RelativeLocationFacingSpace);
}

void FCustomJumpForce::PrepareRootMotion
	(
		float SimulationTime, 
		float MovementTickTime,
		const ACharacter& Character, 
		const UCharacterMovementComponent& MoveComponent
	)
{
	RootMotionParams.Clear();

	if (Duration > SMALL_NUMBER && MovementTickTime > SMALL_NUMBER && SimulationTime > SMALL_NUMBER)
	{
		float CurrentTimeFraction = GetTime() / Duration;
		float TargetTimeFraction = (GetTime() + SimulationTime) / Duration;

		// If we're beyond specified duration, we need to re-map times so that
		// we continue our desired ending velocity
		if (TargetTimeFraction > 1.f)
		{
			float TimeFractionPastAllowable = TargetTimeFraction - 1.0f;
			TargetTimeFraction -= TimeFractionPastAllowable;
			CurrentTimeFraction -= TimeFractionPastAllowable;
		}

		float CurrentMoveFraction = CurrentTimeFraction;
		float TargetMoveFraction = TargetTimeFraction;

		if (TimeMappingCurve)
		{
			CurrentMoveFraction = TimeMappingCurve->GetFloatValue(CurrentTimeFraction);
			TargetMoveFraction = TimeMappingCurve->GetFloatValue(TargetTimeFraction);
		}

		const FVector CurrentRelativeLocation = GetRelativeLocation(CurrentMoveFraction);
		const FVector TargetRelativeLocation = GetRelativeLocation(TargetMoveFraction);

		const FVector Force = (TargetRelativeLocation - CurrentRelativeLocation) / MovementTickTime;

		const FTransform NewTransform(Force);
		RootMotionParams.Set(NewTransform);
	}
	else
	{
		checkf(Duration > SMALL_NUMBER, TEXT("FCustomJumpForce prepared with invalid duration."));
	}

	SetTime(GetTime() + SimulationTime);
}

bool FCustomJumpForce::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!Super::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	Ar << Rotation;
	Ar << Distance;
	Ar << Height;
	Ar << bDisableTimeout;
	Ar << PathOffsetCurve;
	Ar << TimeMappingCurve;

	bOutSuccess = true;
	return true;
}

UScriptStruct* FCustomJumpForce::GetScriptStruct() const
{
	return FCustomJumpForce::StaticStruct();
}

FString FCustomJumpForce::ToSimpleString() const
{
	return FString::Printf(TEXT("[ID:%u]FCustomJumpForce %s"), LocalID, *InstanceName.GetPlainNameString());
}

void FCustomJumpForce::AddReferencedObjects(class FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PathOffsetCurve);
	Collector.AddReferencedObject(TimeMappingCurve);

	Super::AddReferencedObjects(Collector);
}
