#include "PlayerHUD.h"

#include "ActionBar.h"
#include "Buff.h"
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
	if (IsValid(AncientBar))
	{
		AncientBar->InitActionBar(ESaiyoraPlane::Ancient, OwningPlayer);
	}
	if (IsValid(ModernBar))
	{
		ModernBar->InitActionBar(ESaiyoraPlane::Modern, OwningPlayer);
	}
	AddToViewport();
}

void UPlayerHUD::ToggleExtraInfo(const bool bShowExtraInfo)
{
	if (bShowExtraInfo != bDisplayingExtraInfo)
	{
		bDisplayingExtraInfo = bShowExtraInfo;
		OnExtraInfoToggled.Broadcast(bDisplayingExtraInfo);
	}
}

void UPlayerHUD::AddAbilityProc(UBuff* SourceBuff, const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (!IsValid(SourceBuff) || !IsValid(AbilityClass))
	{
		return;
	}
	const UCombatAbility* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
	if (!IsValid(DefaultAbility))
	{
		return;
	}
	switch (DefaultAbility->GetAbilityPlane())
	{
	case ESaiyoraPlane::Ancient :
		if (IsValid(AncientBar))
		{
			AncientBar->AddAbilityProc(SourceBuff, AbilityClass);
		}
		break;
	case ESaiyoraPlane::Modern :
		if (IsValid(ModernBar))
		{
			ModernBar->AddAbilityProc(SourceBuff, AbilityClass);
		}
		break;
	default :
		return;
	}
}