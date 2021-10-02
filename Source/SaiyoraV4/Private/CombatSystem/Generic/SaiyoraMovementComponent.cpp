// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Generic/SaiyoraMovementComponent.h"

#include "SaiyoraCombatInterface.h"
#include "GameFramework/Character.h"

bool USaiyoraMovementComponent::FSavedMove_Saiyora::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	if (bSavedWantsCustomMove != ((FSavedMove_Saiyora*)&NewMove)->bSavedWantsCustomMove)
	{
		return false;
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
	USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement());
	if (Movement)
	{
		Movement->CustomMoveType = SavedMoveType;
		Movement->CustomMoveParams = SavedMoveParams;
		Movement->CustomMoveAbilityID = SavedMoveAbilityID;
	}
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement());
	if (Movement)
	{
		bSavedWantsCustomMove = Movement->bWantsCustomMove;
		SavedMoveType = Movement->CustomMoveType;
		SavedMoveParams = Movement->CustomMoveParams;
		SavedMoveAbilityID = Movement->CustomMoveAbilityID;
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

void USaiyoraMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
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
	}
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void USaiyoraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerAbilityHandler = Cast<UPlayerAbilityHandler>(ISaiyoraCombatInterface::Execute_GetAbilityHandler(GetOwner()));
}

void USaiyoraMovementComponent::CustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType,
                                               FCombatParameters const& Params)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy || !IsValid(Source) || MoveType == ESaiyoraCustomMove::None || !IsValid(OwnerAbilityHandler))
	{
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//Predict movement.
		CustomMoveAbilityID = OwnerAbilityHandler->GetLastPredictionID();
		if (CustomMoveAbilityID == 0)
		{
			return;
		}
		bWantsCustomMove = true;
		CustomMoveType = MoveType;
		CustomMoveParams = Params;
	}
	else if (GetOwnerRole() == ROLE_Authority && PawnOwner->IsLocallyControlled())
	{
		//Perform movement, server auth.
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		//Confirm predicted movement.
	}
}

void USaiyoraMovementComponent::ConfirmCustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType,
	FCombatParameters const& Params)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || MoveType == ESaiyoraCustomMove::None)
	{
		return;
	}
	CustomMoveType = MoveType;
	CustomMoveParams = Params;
}

void USaiyoraMovementComponent::ExecuteCustomMove()
{
	/*if (CustomMoveType == ESaiyoraCustomMove::None)
	{
		return;
	}*/
	FVector Target = FVector::ZeroVector;
	for (FCombatParameter const& Param : CustomMoveParams.Parameters)
	{
		if (Param.ParamType == ECombatParamType::Target)
		{
			Target = Param.Location;
		}
	}
	GetOwner()->SetActorLocation(Target, true);

	CustomMoveType = ESaiyoraCustomMove::None;
	CustomMoveParams = FCombatParameters();
}
