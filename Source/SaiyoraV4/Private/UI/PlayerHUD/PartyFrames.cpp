#include "PartyFrames.h"
#include "PartyFrame.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"
#include "PlayerHUD.h"
#include "VerticalBoxSlot.h"

void UPartyFrames::Init(UPlayerHUD* OwningHUD)
{
	if (!IsValid(OwningHUD))
	{
		return;
	}
	OwnerHUD = OwningHUD;
	ASaiyoraGameState* GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!IsValid(GameStateRef))
	{
		return;
	}
	GameStateRef->OnPlayerAdded.AddDynamic(this, &UPartyFrames::OnPlayerJoined);
	GameStateRef->OnPlayerRemoved.AddDynamic(this, &UPartyFrames::OnPlayerLeft);
	TArray<ASaiyoraPlayerCharacter*> ActivePlayers;
	GameStateRef->GetActivePlayers(ActivePlayers);
	for (ASaiyoraPlayerCharacter* Player : ActivePlayers)
	{
		OnPlayerJoined(Player);
	}
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
	NewFrame->InitFrame(OwnerHUD, PlayerCharacter);
	ActiveFrames.Add(NewFrame);
	UVerticalBoxSlot* NewFrameSlot = PartyFramesVBox->AddChildToVerticalBox(NewFrame);
	if (IsValid(NewFrameSlot))
	{
		NewFrameSlot->SetHorizontalAlignment(HAlign_Center);
		NewFrameSlot->SetVerticalAlignment(VAlign_Top);
	}
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