#include "CoreClasses/SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"

ASaiyoraGameState::ASaiyoraGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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

void ASaiyoraGameState::RemovePlayer(ASaiyoraPlayerCharacter* Player)
{
	ActivePlayers.Remove(Player);
	OnPlayerRemoved.Broadcast(Player);
}