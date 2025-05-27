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

	//We just brute force check for players becoming invalid.
	//Since we're managing our PlayerArray locally on every machine (and waiting for refs to become valid before adding them to the array),
	//only the server would be able to rely on things like PostLogin or OnLogout from the GameMode.
	for (int i = ActivePlayers.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(ActivePlayers[i]))
		{
			ActivePlayers.RemoveAt(i);
			OnPlayerRemoved.Broadcast();
		}
	}
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