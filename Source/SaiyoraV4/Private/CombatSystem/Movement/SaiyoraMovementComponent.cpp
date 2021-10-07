// Fill out your copyright notice in the Description page of Project Settings.

#include "Movement/SaiyoraMovementComponent.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/Character.h"
#include "Movement/MovementStructs.h"

bool USaiyoraMovementComponent::FSavedMove_Saiyora::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_Saiyora* CastMove = (FSavedMove_Saiyora*)&NewMove;
	if (CastMove)
	{
		if (bSavedWantsCustomMove != CastMove->bSavedWantsCustomMove || SavedMoveType != CastMove->SavedMoveType)
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
		Movement->CustomMoveType = SavedMoveType;
		Movement->CustomMoveAbilityRequest = SavedAbilityRequest;
		Movement->DirectionTeleportInfo = SavedDirectionTeleportInfo;
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
		SavedMoveType = Movement->CustomMoveType;
		SavedAbilityRequest = Movement->CustomMoveAbilityRequest;
		SavedDirectionTeleportInfo = Movement->DirectionTeleportInfo;
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
		CustomMoveType = CastMove->SavedMoveType;
		CustomMoveAbilityRequest = CastMove->SavedAbilityRequest;
	}
}

bool USaiyoraMovementComponent::FSaiyoraNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement,
	FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	Ar << CustomMoveType;
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
		CustomMoveType = ESaiyoraCustomMove::None;
		CustomMoveAbilityRequest.AbilityClass = nullptr;
		CustomMoveAbilityRequest.PredictionID = 0;
		CustomMoveAbilityRequest.Tick = 0;
		CustomMoveAbilityRequest.PredictionParams.Parameters.Empty();
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
		CustomMoveType = MoveData->CustomMoveType;
		if (CustomMoveType != ESaiyoraCustomMove::None)
		{
			//UE_LOG(LogTemp, Warning, TEXT("What the fuck unreal?"));
			CustomMoveAbilityRequest = MoveData->CustomMoveAbilityRequest;
		}
	}
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void USaiyoraMovementComponent::ExecuteCustomMove()
{
	//Check against invalid move type.
	if (CustomMoveType == ESaiyoraCustomMove::None)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
	{
		//If we are the server, and have an auto proxy, check that this move hasn't already been sent (duplicating cast IDs), and can actually be performed.
		if (ServerCompletedMovementIDs.Contains(CustomMoveAbilityRequest.PredictionID) || !IsValid(OwnerAbilityHandler) || !OwnerAbilityHandler->UseAbilityFromPredictedMovement(CustomMoveAbilityRequest))
		{
			return;
		}
	}
	//Check that this move succeeded on clients. If a misprediction occurs, this prevents the move from being replayed over and over.
	if (GetOwnerRole() == ROLE_AutonomousProxy && !CompletedCastStatus.FindRef(CustomMoveAbilityRequest.PredictionID))
	{
		return;
	}
	
	//Perform move!
	switch(CustomMoveType)
	{
		case ESaiyoraCustomMove::None :
			break;
		case ESaiyoraCustomMove::Teleport :
			ExecuteTeleportInDirection(DirectionTeleportInfo);
			DirectionTeleportInfo = FDirectionTeleportInfo();
			break;
		return;
	}

	//For the server, if we have an auto proxy, document that this prediction ID has been used already.
	if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
	{
		ServerCompletedMovementIDs.Add(CustomMoveAbilityRequest.PredictionID);
	}
}

void USaiyoraMovementComponent::AbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason)
{
	CompletedCastStatus.Add(PredictionID, false);
}

void USaiyoraMovementComponent::TeleportInDirection(UPlayerCombatAbility* Source, float const Length, bool const bShouldSweep, bool const bIgnoreZ, FCombatParameters const& PredictionParams)
{
	if (!IsValid(Source) || GetOwnerRole() == ROLE_SimulatedProxy || !IsValid(OwnerAbilityHandler) || OwnerAbilityHandler->GetCastingState().ElapsedTicks != 0)
	{
		return;
	}
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		FDirectionTeleportInfo TeleInfo;
		for (FCombatParameter const& Param : PredictionParams.Parameters)
		{
			if (Param.ParamName == "Direction")
			{
				TeleInfo.Direction = Param.VectorParam;
			}
		}
		TeleInfo.Length = Length;
		TeleInfo.bShouldSweep = bShouldSweep;
		TeleInfo.bIgnoreZ = bIgnoreZ;
		ExecuteTeleportInDirection(TeleInfo);
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		int32 const PredictionID = OwnerAbilityHandler->GetLastPredictionID();
		if (CompletedCastStatus.Contains(PredictionID))
		{
			return;
		}
		CompletedCastStatus.Add(PredictionID, true);
		bWantsCustomMove = true;
		CustomMoveType = ESaiyoraCustomMove::Teleport;
		CustomMoveAbilityRequest.PredictionID = OwnerAbilityHandler->GetLastPredictionID();
		CustomMoveAbilityRequest.PredictionParams = PredictionParams;
		CustomMoveAbilityRequest.Tick = 0;
		CustomMoveAbilityRequest.AbilityClass = Source->GetClass();
		CustomMoveAbilityRequest.ClientStartTime = GameStateRef->GetServerWorldTimeSeconds();
		for (FCombatParameter const& Param : PredictionParams.Parameters)
		{
			if (Param.ParamName == "Direction")
			{
				DirectionTeleportInfo.Direction = Param.VectorParam;
			}
		}
		DirectionTeleportInfo.Length = Length;
		DirectionTeleportInfo.bShouldSweep = bShouldSweep;
		DirectionTeleportInfo.bIgnoreZ = bIgnoreZ;
	}
}

void USaiyoraMovementComponent::ExecuteTeleportInDirection(FDirectionTeleportInfo const& TeleportInfo)
{
	FVector const OriginalLocation = GetOwner()->GetActorLocation();
	FVector Travel = TeleportInfo.Direction;
	if (TeleportInfo.bIgnoreZ)
	{
		Travel.Z = 0.0f;
	}
	Travel.Normalize();
	FVector const TargetLocation = OriginalLocation + (Travel * TeleportInfo.Length);
	GetOwner()->SetActorLocation(TargetLocation, TeleportInfo.bShouldSweep);
}
