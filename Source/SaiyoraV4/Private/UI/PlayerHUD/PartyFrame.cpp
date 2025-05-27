#include "PartyFrame.h"
#include "SaiyoraPlayerCharacter.h"

void UPartyFrame::InitFrame(ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player))
	{
		return;
	}
	PlayerCharacter = Player;
}