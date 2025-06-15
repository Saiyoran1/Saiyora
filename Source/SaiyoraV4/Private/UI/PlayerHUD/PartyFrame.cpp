#include "PartyFrame.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "BuffIcon.h"
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

	if (UBuffHandler* BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(Player))
	{
		BuffHandler->OnIncomingBuffApplied.AddDynamic(this, &UPartyFrame::OnBuffApplied);
		BuffHandler->OnIncomingBuffRemoved.AddDynamic(this, &UPartyFrame::OnBuffRemoved);
		TArray<UBuff*> ActiveBuffs;
		BuffHandler->GetBuffs(ActiveBuffs);
		for (UBuff* Buff : ActiveBuffs)
		{
			OnBuffApplied(Buff);
		}
	}
}

void UPartyFrame::OnBuffApplied(UBuff* Buff)
{
	if (!IsValid(Buff) || !IsValid(BuffIconClass))
	{
		return;
	}
	if (!Buff->ShouldDisplayOnPartyFrame())
	{
		return;
	}
	UBuffIcon* NewIcon = CreateWidget<UBuffIcon>(this, BuffIconClass);
	if (!IsValid(NewIcon))
	{
		return;
	}
	BuffIcons.Add(Buff, NewIcon);
	BuffBox->AddChildToHorizontalBox(NewIcon);
	NewIcon->Init(Buff);
}

void UPartyFrame::OnBuffRemoved(const FBuffRemoveEvent& RemoveEvent)
{
	UBuffIcon* Icon = BuffIcons.FindRef(RemoveEvent.RemovedBuff);
	if (IsValid(Icon))
	{
		Icon->Cleanup();
		Icon->RemoveFromParent();
		BuffIcons.Remove(RemoveEvent.RemovedBuff);
	}
}