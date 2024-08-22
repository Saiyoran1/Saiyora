#include "PlayerHUD.h"
#include "BuffContainer.h"
#include "CastBar.h"
#include "DungeonDisplay.h"
#include "HealthBar.h"
#include "ResourceContainer.h"
#include "SaiyoraPlayerCharacter.h"

void UPlayerHUD::InitializePlayerHUD(ASaiyoraPlayerCharacter* OwnerPlayer)
{
	if (!IsValid(OwnerPlayer))
	{
		return;
	}
	OwningPlayer = OwnerPlayer;
	if (IsValid(HealthBar))
	{
		HealthBar->InitHealthBar(this, OwningPlayer);
	}
	if (IsValid(BuffContainer))
	{
		BuffContainer->InitBuffContainer(this, OwningPlayer, EBuffType::Buff);
	}
	if (IsValid(DebuffContainer))
	{
		DebuffContainer->InitBuffContainer(this, OwningPlayer, EBuffType::Debuff);
	}
	if (IsValid(DungeonDisplay))
	{
		DungeonDisplay->InitDungeonDisplay(this);
	}
	if (IsValid(CastBar))
	{
		CastBar->InitCastBar(OwningPlayer);
	}
	if (IsValid(ResourceContainer))
	{
		ResourceContainer->InitResourceContainer(OwningPlayer);
	}
}

void UPlayerHUD::ToggleExtraInfo(const bool bShowExtraInfo)
{
	if (bShowExtraInfo != bDisplayingExtraInfo)
	{
		bDisplayingExtraInfo = bShowExtraInfo;
		OnExtraInfoToggled.Broadcast(bDisplayingExtraInfo);
	}
}