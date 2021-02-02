// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraPlayerController.h"

#include "SaiyoraGameState.h"

void ASaiyoraPlayerController::AcknowledgePossession(APawn* P)
{
    Super::AcknowledgePossession(P);
    
    if (IsValid(P) && GetLocalRole() != ROLE_Authority && IsLocalPlayerController())
    {
        OnClientPossess(P);
    }
}

void ASaiyoraPlayerController::BeginPlay()
{
    Super::BeginPlay();

    GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
    
    if (IsLocalController())
    {
        GetWorld()->GetTimerManager().SetTimer(WorldTimeRequestHandle, this, &ASaiyoraPlayerController::RequestWorldTime, 1.0f, true);
    }
}

void ASaiyoraPlayerController::RequestWorldTime()
{
    ServerHandleWorldTimeRequest(GameStateRef->GetWorldTime(), Ping);
}

void ASaiyoraPlayerController::ServerHandleWorldTimeRequest_Implementation(float const ClientTime, float const LastPing)
{
    ClientHandleWorldTimeReturn(ClientTime, GameStateRef->GetWorldTime());
}

bool ASaiyoraPlayerController::ServerHandleWorldTimeRequest_Validate(float const ClientTime, float const LastPing)
{
    return true;
}

void ASaiyoraPlayerController::ClientHandleWorldTimeReturn_Implementation(float const ClientTime, float const ServerTime)
{
    Ping = FMath::Max(0.0f, (GameStateRef->GetWorldTime() - ClientTime));
    GameStateRef->UpdateClientWorldTime(ServerTime);
}
