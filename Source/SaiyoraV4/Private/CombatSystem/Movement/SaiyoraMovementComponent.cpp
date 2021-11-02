// Fill out your copyright notice in the Description page of Project Settings.

#include "Movement/SaiyoraMovementComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
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
		if (bSavedWantsCustomMove != CastMove->bSavedWantsCustomMove || SavedPendingCustomMove.MoveParams.MoveType != CastMove->SavedPendingCustomMove.MoveParams.MoveType)
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
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	if (RepFlags->bNetOwner)
	{
		if (HandlersAwaitingPingDelay.Num() > 0)
		{
			bWroteSomething |= Channel->ReplicateSubobjectList(HandlersAwaitingPingDelay, *Bunch, *RepFlags);
		}
		if (ReplicatedRootMotionHandlers.Num() > 0)
		{
			for (USaiyoraRootMotionHandler* Handler : ReplicatedRootMotionHandlers)
			{
				if (IsValid(Handler) && Handler->GetPredictionID() == 0)
				{
					bWroteSomething |= Channel->ReplicateSubobject(Handler, *Bunch, *RepFlags);
				}
			}
		}
	}
	else if (ReplicatedRootMotionHandlers.Num() > 0)
	{
		bWroteSomething |= Channel->ReplicateSubobjectList(ReplicatedRootMotionHandlers, *Bunch, *RepFlags);
	}
	return bWroteSomething;
}

void USaiyoraMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	return;
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
		CustomMoveFromFlag();
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
	CurrentTickServerCustomMoveSources.Empty();
}

void USaiyoraMovementComponent::CustomMoveFromFlag()
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
		ExecuteCustomMove(PendingCustomMove.MoveParams);
	}
}

void USaiyoraMovementComponent::ExecuteCustomMove(FCustomMoveParams const& CustomMove)
{
	if (GetOwnerRole() == ROLE_Authority || GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//TODO: Check can use move.
	}
	switch (CustomMove.MoveType)
	{
		case ESaiyoraCustomMove::Launch :
			ExecuteLaunchPlayer(CustomMove.Target);
			break;
		case ESaiyoraCustomMove::TeleportLocation :
			ExecuteTeleportToLocation(CustomMove.Target, CustomMove.Rotation);
			break;
		default:
			break;
	}
}

void USaiyoraMovementComponent::AbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason)
{
	CompletedCastStatus.Add(PredictionID, false);
}

void USaiyoraMovementComponent::SetupCustomMovementPrediction(UPlayerCombatAbility* Source, FCustomMoveParams const& CustomMove)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || CustomMove.MoveType == ESaiyoraCustomMove::None || !IsValid(Source) || !IsValid(OwnerAbilityHandler))
	{
		return;
	}
	OwnerAbilityHandler->SubscribeToAbilityTicked(OnPredictedAbility);
	PendingCustomMove.AbilityClass = Source->GetClass();
	PendingCustomMove.MoveParams = CustomMove;
	PendingCustomMove.PredictionID = Source->GetCurrentPredictionID();
	PendingCustomMove.OriginalTimestamp = GameStateRef->GetServerWorldTimeSeconds();
}

void USaiyoraMovementComponent::OnCustomMoveCastPredicted(FCastEvent const& Event)
{
	OwnerAbilityHandler->UnsubscribeFromAbilityTicked(OnPredictedAbility);
	CompletedCastStatus.Add(Event.PredictionID, true);
	if (!IsValid(PendingCustomMove.AbilityClass) || PendingCustomMove.AbilityClass != Event.Ability->GetClass()
		|| PendingCustomMove.PredictionID != Event.PredictionID || PendingCustomMove.MoveParams.MoveType == ESaiyoraCustomMove::None
			|| Event.Tick != 0)
	{
		PendingCustomMove.Clear();
		return;
	}
	PendingCustomMove.PredictionParams = Event.PredictionParams;
	bWantsCustomMove = true;
}

bool USaiyoraMovementComponent::ApplyCustomMove(FCustomMoveParams const& CustomMove, UObject* Source)
{
	if (CustomMove.MoveType == ESaiyoraCustomMove::None || !IsValid(Source))
	{
		return false;
	}
	UPlayerCombatAbility* PlayerAbilitySource = Cast<UPlayerCombatAbility>(Source);
	if (!IsValid(PlayerAbilitySource) || PlayerAbilitySource->GetHandler()->GetOwner() != GetOwner() || PlayerAbilitySource->GetCurrentPredictionID() == 0)
	{
		//This move should NOT deal with prediction, and thus must be on the server.
		if (GetOwnerRole() != ROLE_Authority)
		{
			return false;
		}
		//Listen servers call Predicted and Server ability ticks back to back, so we need to guard against getting the same input twice in one tick.
		if (CurrentTickServerCustomMoveSources.Contains(Source))
		{
			return false;
		}
		CurrentTickServerCustomMoveSources.Add(Source);
		
		if (PawnOwner->IsLocallyControlled())
		{
			Multicast_ExecuteCustomMove(CustomMove);
		}
		else
		{
			//Set timer to apply custom move after ping delay.
			FTimerDelegate PingDelayDelegate;
			//TODO: Investigate why I can't pass a ref to a struct into a timer delegate.
			PingDelayDelegate.BindUObject(this, &USaiyoraMovementComponent::DelayedCustomMoveApplication, CustomMove);
			FTimerHandle PingDelayHandle;
			float const PingDelay = USaiyoraCombatLibrary::GetActorPing(GetOwner());
			GetWorld()->GetTimerManager().SetTimer(PingDelayHandle, PingDelayDelegate, PingDelay, false);
			Client_ExecuteCustomMove(CustomMove);
		}
		return true;
	}
	//This move came from a player ability, was initiated by the player on itself, and has a valid prediction ID.
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
			Multicast_ExecuteCustomMoveNoOwner(CustomMove);
			return true;
		}
	case ROLE_AutonomousProxy :
		{
			//Since we know this came from an ability, it can be predicted.
			SetupCustomMovementPrediction(PlayerAbilitySource, CustomMove);
			return true;
		}
	case ROLE_SimulatedProxy :
		{
			//Sim proxies will get custom moves via RPC from the server.
			return false;
		}
	default :
		return false;
	}
}

void USaiyoraMovementComponent::DelayedCustomMoveApplication(FCustomMoveParams CustomMove)
{
	//TODO: Recheck move can be executed? This could lead to situations where being rooted during ping delay actually prevents movements.
	Multicast_ExecuteCustomMoveNoOwner(CustomMove);
}

void USaiyoraMovementComponent::Client_ExecuteCustomMove_Implementation(FCustomMoveParams const& CustomMove)
{
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::Multicast_ExecuteCustomMoveNoOwner_Implementation(FCustomMoveParams const& CustomMove)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		return;
	}
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::Multicast_ExecuteCustomMove_Implementation(FCustomMoveParams const& CustomMove)
{
	ExecuteCustomMove(CustomMove);
}

bool USaiyoraMovementComponent::TeleportToLocation(UObject* Source, FVector const& Target, FRotator const& DesiredRotation)
{
	FCustomMoveParams TeleParams;
	TeleParams.MoveType = ESaiyoraCustomMove::TeleportLocation;
	TeleParams.Target = Target;
	TeleParams.Rotation = DesiredRotation;
	TeleParams.bStopMovement = true;
	return ApplyCustomMove(TeleParams, Source);
}

void USaiyoraMovementComponent::ExecuteTeleportToLocation(FVector const& Target, FRotator const& DesiredRotation)
{
	GetOwner()->SetActorLocation(Target);
	GetOwner()->SetActorRotation(DesiredRotation);
}

bool USaiyoraMovementComponent::LaunchPlayer(UPlayerCombatAbility* Source, FVector const& LaunchVector)
{
	FCustomMoveParams LaunchParams;
	LaunchParams.MoveType = ESaiyoraCustomMove::Launch;
	LaunchParams.Target = LaunchVector;
	return ApplyCustomMove(LaunchParams, Source);
}

void USaiyoraMovementComponent::ExecuteLaunchPlayer(FVector const& LaunchVector)
{
	Launch(LaunchVector);
}

void USaiyoraMovementComponent::ApplyJumpForce(UObject* Source, ERootMotionAccumulateMode const AccumulateMode,
	int32 const Priority, float const Duration, FRotator const& Rotation, float const Distance, float const Height,
	bool const bFinishOnLanded, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve)
{
	UJumpForceHandler* JumpForce = NewObject<UJumpForceHandler>(GetOwner(), UJumpForceHandler::StaticClass());
	if (!IsValid(JumpForce))
	{
		return;
	}
	JumpForce->AccumulateMode = AccumulateMode;
	JumpForce->Priority = Priority;
	JumpForce->Duration = Duration;
	JumpForce->Rotation = Rotation;
	JumpForce->Distance = Distance;
	JumpForce->Height = Height;
	JumpForce->bFinishOnLanded = bFinishOnLanded;
	JumpForce->PathOffsetCurve = PathOffsetCurve;
	JumpForce->TimeMappingCurve = TimeMappingCurve;
	JumpForce->FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	//Can add the option to clamp velocity at the end I guess if needed.
	/*JumpForce->FinishSetVelocity = FVector::ZeroVector;
	JumpForce->FinishClampVelocity = 0.0f;*/
	ApplyCustomRootMotionHandler(JumpForce, Source);
}

void USaiyoraMovementComponent::ApplyConstantForce(UObject* Source, ERootMotionAccumulateMode const AccumulateMode, int32 const Priority,
	float const Duration, FVector const& Force, UCurveFloat* StrengthOverTime)
{
	UConstantForceHandler* ConstantForce = NewObject<UConstantForceHandler>(GetOwner(), UConstantForceHandler::StaticClass());
	if (!IsValid(ConstantForce))
	{
		return;
	}
	ConstantForce->AccumulateMode = AccumulateMode;
	ConstantForce->Priority = Priority;
	ConstantForce->Duration = Duration;
	ConstantForce->Force = Force;
	ConstantForce->StrengthOverTime = StrengthOverTime;
	ConstantForce->FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	ApplyCustomRootMotionHandler(ConstantForce, Source);
}

void USaiyoraMovementComponent::RemoveRootMotionHandler(USaiyoraRootMotionHandler* Handler)
{
	if (!IsValid(Handler))
	{
		return;
	}
	RemoveRootMotionSourceByID(Handler->GetHandledID());
	CurrentRootMotionHandlers.Remove(Handler);
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(Handler->PingDelayHandle))
		{
			HandlersAwaitingPingDelay.Remove(Handler);
			Handler->PingDelayHandle.Invalidate();
		}
		else
		{
			ReplicatedRootMotionHandlers.Remove(Handler);
		}
	}
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
			float const PingDelay = USaiyoraCombatLibrary::GetActorPing(GetOwner());
			GetWorld()->GetTimerManager().SetTimer(PingDelayHandle, PingDelayDelegate, PingDelay, false);
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
			FCustomMoveParams TempParams;
			TempParams.MoveType = ESaiyoraCustomMove::RootMotion;
			SetupCustomMovementPrediction(PlayerAbilitySource, TempParams);
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