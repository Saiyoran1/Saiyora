// Fill out your copyright notice in the Description page of Project Settings.

#include "Movement/SaiyoraMovementComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Character.h"
#include "Movement/MovementStructs.h"

float const USaiyoraMovementComponent::MaxPingDelay = 0.2f;

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
	SetIsReplicatedByDefault(true);
}

bool USaiyoraMovementComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = false;
	if (RepFlags->bNetOwner)
	{
		bWroteSomething |= Channel->ReplicateSubobjectList(HandlersAwaitingPingDelay, *Bunch, *RepFlags);
		for (USaiyoraRootMotionHandler* Handler : ReplicatedRootMotionHandlers)
		{
			if (IsValid(Handler) && Handler->GetPredictionID() == 0)
			{
				bWroteSomething |= Channel->ReplicateSubobject(Handler, *Bunch, *RepFlags);
			}
		}
	}
	else
	{
		bWroteSomething |= Channel->ReplicateSubobjectList(ReplicatedRootMotionHandlers, *Bunch, *RepFlags);
	}
	bWroteSomething |= Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	return bWroteSomething;
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

void USaiyoraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Clear this every tick, as it only exists to prevent listen servers from double-applying Root Motion Sources.
	//This happens because the ability system calls Predicted and Server tick of an ability back to back on listen servers.
	CurrentTickServerRootMotionSources.Empty();
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
	PendingCustomMove.PredictionID = Source->GetCurrentPredictionID();
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

void USaiyoraMovementComponent::TestRootMotion(UObject* Source)
{
	UTestJumpForceHandler* JumpForce = NewObject<UTestJumpForceHandler>(GetOwner(), UTestJumpForceHandler::StaticClass());
	if (!IsValid(JumpForce))
	{
		return;
	}
	JumpForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	JumpForce->Priority = 500;
	JumpForce->Duration = 1.0f;
	JumpForce->Rotation = GetOwner()->GetActorRotation();
	JumpForce->Distance = 500.0f;
	JumpForce->Height = 1000.0f;
	JumpForce->bFinishOnLanded = false;
	JumpForce->PathOffsetCurve = nullptr;
	JumpForce->TimeMappingCurve = nullptr;
	JumpForce->FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	JumpForce->FinishSetVelocity = FVector::ZeroVector;
	JumpForce->FinishClampVelocity = 0.0f;
	ApplyCustomRootMotionHandler(JumpForce, Source);
}

void USaiyoraMovementComponent::RemoveRootMotionHandler(USaiyoraRootMotionHandler* Handler)
{
	if (!IsValid(Handler))
	{
		return;
	}
	RemoveRootMotionSourceByID(Handler->GetHandledID());
	CurrentRootMotionHandlers.Remove(Handler);
	//TODO: Figure out how to do cleanup with replication.
	/*
	if (GetWorld()->GetTimerManager().IsTimerActive(Handler->PingDelayHandle))
	{
		HandlersAwaitingPingDelay.Remove(Handler);
		Handler->PingDelayHandle.Invalidate();
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		ReplicatedRootMotionHandlers.Remove(Handler);
		HandlersAwaitingPingDelay.Remove(Handler);
	}*/
}

bool USaiyoraMovementComponent::ApplyCustomRootMotionHandler(USaiyoraRootMotionHandler* Handler, UObject* Source)
{
	if (!IsValid(Source))
	{
		return false;
	}
	UPlayerCombatAbility* PlayerAbilitySource = Cast<UPlayerCombatAbility>(Source);
	if (!IsValid(PlayerAbilitySource) || PlayerAbilitySource->GetHandler()->GetOwner() != GetOwner() || PlayerAbilitySource->GetCurrentPredictionID() == 0)
	{
		//This root motion source should NOT deal with prediction, and thus must be on the server.
		if (GetOwnerRole() != ROLE_Authority)
		{
			return false;
		}
		//Listen servers call Predicted and Server ability ticks back to back, so we need to guard against getting the same input twice in one tick.
		if (CurrentTickServerRootMotionSources.Contains(Source))
		{
			return false;
		}
		CurrentTickServerRootMotionSources.Add(Source);
		
		if (PawnOwner->IsLocallyControlled())
		{
			Handler->Init(this, 0, Source);
			CurrentRootMotionHandlers.Add(Handler);
			ReplicatedRootMotionHandlers.Add(Handler);
			Handler->Apply();
		}
		else
		{
			Handler->Init(this, 0, Source);
			HandlersAwaitingPingDelay.Add(Handler);
			//Set timer to move handler from awaiting to replicated, add to current, then call Apply().
			FTimerDelegate PingDelayDelegate;
			PingDelayDelegate.BindUObject(this, &USaiyoraMovementComponent::DelayedHandlerApplication, Handler);
			FTimerHandle PingDelayHandle;
			GetWorld()->GetTimerManager().SetTimer(PingDelayHandle, PingDelayDelegate, FMath::Min(MaxPingDelay, USaiyoraCombatLibrary::GetActorPing(GetOwner())), false);
			//Store the timer handle. If something clears the movement before ping (very unlikely), we can cancel the timer.
			Handler->PingDelayHandle = PingDelayHandle;
		}
		//This should instantly replicate the new handler to the clients.
		GetOwner()->ForceNetUpdate();
		return true;
	}
	//This root motion came from a player ability, was initiated by the player on itself, and has a valid prediction ID.
	switch (GetOwnerRole())
	{
	case ROLE_Authority :
		{
			//There are going to be 2 calls to predicted movement (one from the ability system, one from the Networked Move Data), so we need to check that this is the first call (the 2nd won't do anything).
			if (ServerCompletedMovementIDs.Contains(PlayerAbilitySource->GetCurrentPredictionID()))
			{
				return false;
			}
			ServerCompletedMovementIDs.Add(PlayerAbilitySource->GetCurrentPredictionID());
			Handler->Init(this, PlayerAbilitySource->GetCurrentPredictionID(), Source);
			CurrentRootMotionHandlers.Add(Handler);
			ReplicatedRootMotionHandlers.Add(Handler);
			//No delayed apply here, since the client should already have predicted this.
			Handler->Apply();
			return true;
		}
	case ROLE_AutonomousProxy :
		{
			//Since we know this came from an ability, it can be predicted.
			SetupCustomMovement(PlayerAbilitySource, ESaiyoraCustomMove::RootMotion, FCustomMoveParams());
			Handler->Init(this, PlayerAbilitySource->GetCurrentPredictionID(), Source);
			CurrentRootMotionHandlers.Add(Handler);
			//Note that we don't add to rep arrays on auto proxy. There's no need.
			Handler->Apply();
			return true;
		}
	case ROLE_SimulatedProxy :
		{
			//Sim proxies shouldn't ever need to create their own root motion source handler.
			//They will get a replicated version from the server.
			return false;
		}
	default :
		return false;
	}
}

void USaiyoraMovementComponent::DelayedHandlerApplication(USaiyoraRootMotionHandler* Handler)
{
	if (!IsValid(Handler))
	{
		return;
	}
	if (HandlersAwaitingPingDelay.Remove(Handler) > 0)
	{
		CurrentRootMotionHandlers.Add(Handler);
		ReplicatedRootMotionHandlers.Add(Handler);
		Handler->Apply();
	}
}

void USaiyoraMovementComponent::AddRootMotionHandlerFromReplication(USaiyoraRootMotionHandler* Handler)
{
	if (IsValid(Handler))
	{
		CurrentRootMotionHandlers.Add(Handler);
		ReplicatedRootMotionHandlers.Add(Handler);
	}
}