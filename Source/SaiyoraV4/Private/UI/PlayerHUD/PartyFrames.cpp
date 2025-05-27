#include "PartyFrames.h"
#include "PartyFrame.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"

void UPartyFrames::Init()
{
	ASaiyoraGameState* GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!IsValid(GameStateRef))
	{
		return;
	}
	GameStateRef->OnPlayerAdded.AddDynamic(this, &UPartyFrames::OnPlayerJoined);
	GameStateRef->OnPlayerRemoved.AddDynamic(this, &UPartyFrames::OnPlayerLeft);
}

void UPartyFrames::OnPlayerJoined(ASaiyoraPlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter) || !IsValid(PartyFrameClass))
	{
		return;
	}
	UPartyFrame* NewFrame = CreateWidget<UPartyFrame>(this, PartyFrameClass);
	if (!IsValid(NewFrame))
	{
		return;
	}
	NewFrame->InitFrame(PlayerCharacter);
	ActiveFrames.Add(NewFrame);
	PartyFramesVBox->AddChildToVerticalBox(NewFrame);
}

void UPartyFrames::OnPlayerLeft(ASaiyoraPlayerCharacter* PlayerCharacter)
{
	//Check which of the party frames is pointing to a player that left and remove it.
	for (int i = ActiveFrames.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(ActiveFrames[i]->GetAssignedPlayer()) || ActiveFrames[i]->GetAssignedPlayer() == PlayerCharacter)
		{
			ActiveFrames[i]->RemoveFromParent();
			ActiveFrames.RemoveAt(i);
		}
	}
}