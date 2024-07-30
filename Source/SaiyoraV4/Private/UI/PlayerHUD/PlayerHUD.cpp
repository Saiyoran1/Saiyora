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
		HealthBar->InitHealthBar(OwningPlayer);
	}
}