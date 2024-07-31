#include "PlayerHUD.h"
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
}

void UPlayerHUD::ToggleExtraInfo(const bool bShowExtraInfo)
{
	if (bShowExtraInfo != bDisplayingExtraInfo)
	{
		bDisplayingExtraInfo = bShowExtraInfo;
		OnExtraInfoToggled.Broadcast(bDisplayingExtraInfo);
	}
}