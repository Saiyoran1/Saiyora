#include "CoreClasses/SaiyoraGameState.h"
#include "Hitbox.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraPlayerCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

ASaiyoraGameState::ASaiyoraGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASaiyoraGameState::BeginPlay()
{
	Super::BeginPlay();

	GameInstanceRef = Cast<USaiyoraGameInstance>(GetGameInstance());
}

void ASaiyoraGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WorldTime += DeltaSeconds;
}

void ASaiyoraGameState::InitPlayer(ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || ActivePlayers.Contains(Player))
	{
		return;
	}
	ActivePlayers.Add(Player);
	OnPlayerAdded.Broadcast(Player);
}