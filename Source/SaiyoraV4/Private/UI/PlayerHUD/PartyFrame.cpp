#include "PartyFrame.h"
#include "PlayerHUD.h"
#include "HealthBar.h"
#include "SaiyoraPlayerCharacter.h"

void UPartyFrame::InitFrame(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || !IsValid(OwningHUD))
	{
		return;
	}
	PlayerCharacter = Player;
	OwnerHUD = OwningHUD;
	
	HealthBar->InitHealthBar(OwningHUD, Player);
}