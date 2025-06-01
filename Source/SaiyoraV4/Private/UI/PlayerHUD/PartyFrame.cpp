#include "PartyFrame.h"
#include "PlayerHUD.h"
#include "HealthBar.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/PlayerState.h"

void UPartyFrame::InitFrame(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || !IsValid(OwningHUD))
	{
		return;
	}
	PlayerCharacter = Player;
	OwnerHUD = OwningHUD;
	
	HealthBar->InitHealthBar(OwningHUD, Player);
	
	FString PlayerName = Player->GetPlayerState()->GetPlayerName();
	if (PlayerName.Len() > PlayerNameCharLimit)
	{
		PlayerName = PlayerName.Left(PlayerNameCharLimit);
		PlayerName.Append("...");
	}
	NameText->SetText(FText::FromString(PlayerName));
}