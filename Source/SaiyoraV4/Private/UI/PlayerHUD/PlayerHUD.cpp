#include "PlayerHUD.h"

#include "BuffContainer.h"
#include "HealthBar.h"
#include "SaiyoraPlayerCharacter.h"

void UPlayerHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningPlayerPawn());
	if (!IsValid(OwningPlayer))
	{
		return;
	}

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
}

void UPlayerHUD::ToggleExtraInfo(const bool bShowExtraInfo)
{
	if (bShowExtraInfo != bDisplayingExtraInfo)
	{
		bDisplayingExtraInfo = bShowExtraInfo;
		OnExtraInfoToggled.Broadcast(bDisplayingExtraInfo);
	}
}