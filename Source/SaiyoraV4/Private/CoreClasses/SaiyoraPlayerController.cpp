#include "SaiyoraPlayerController.h"
#include "SaiyoraPlayerCharacter.h"
#include "CoreClasses/SaiyoraGameState.h"

void ASaiyoraPlayerController::AcknowledgePossession(APawn* P)
{
    Super::AcknowledgePossession(P);
    if (GetPawn())
    {
        PlayerCharacterRef = Cast<ASaiyoraPlayerCharacter>(GetPawn());
    }
}

void ASaiyoraPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (GetPawn())
    {
        PlayerCharacterRef = Cast<ASaiyoraPlayerCharacter>(GetPawn());
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
    ServerHandleWorldTimeRequest(GameStateRef->GetServerWorldTimeSeconds(), Ping);
}

void ASaiyoraPlayerController::ServerHandleWorldTimeRequest_Implementation(float const ClientTime, float const LastPing)
{
    ClientHandleWorldTimeReturn(ClientTime, GameStateRef->GetServerWorldTimeSeconds());
}

bool ASaiyoraPlayerController::ServerHandleWorldTimeRequest_Validate(float const ClientTime, float const LastPing)
{
    return true;
}

void ASaiyoraPlayerController::ClientHandleWorldTimeReturn_Implementation(float const ClientTime, float const ServerTime)
{
    Ping = FMath::Max(0.0f, (GameStateRef->GetServerWorldTimeSeconds() - ClientTime));
    OnPingChanged.Broadcast(Ping);
    GameStateRef->UpdateClientWorldTime(ServerTime + (Ping / 2.0f));
    ServerFinalPingBounce(ServerTime);
}

void ASaiyoraPlayerController::ServerFinalPingBounce_Implementation(float const ServerTime)
{
    Ping = FMath::Max(0.0f, GameStateRef->GetServerWorldTimeSeconds() - ServerTime);
    OnPingChanged.Broadcast(Ping);
}

bool ASaiyoraPlayerController::ServerFinalPingBounce_Validate(float const ServerTime)
{
    return true;
}