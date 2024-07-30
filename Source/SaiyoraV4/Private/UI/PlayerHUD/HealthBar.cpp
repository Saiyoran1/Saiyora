#include "PlayerHUD/HealthBar.h"
#include "DamageHandler.h"
#include "ProgressBar.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraPlayerCharacter.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"

void UHealthBar::InitHealthBar(ASaiyoraPlayerCharacter* OwningPlayer)
{
	if (!IsValid(OwningPlayer) || !OwningPlayer->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	UIDataAsset = Cast<USaiyoraGameInstance>(GetGameInstance())->UIDataAsset;
	if (IsValid(UIDataAsset))
	{
		HealthBar->SetFillColorAndOpacity(UIDataAsset->PlayerHealthColor);
		AbsorbBar->SetFillColorAndOpacity(UIDataAsset->AbsorbOutlineColor);
	}
	OwnerDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(OwningPlayer);
	if (IsValid(OwnerDamageHandler))
	{
		OwnerDamageHandler->OnHealthChanged.AddDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnMaxHealthChanged.AddDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnAbsorbChanged.AddDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnLifeStatusChanged.AddDynamic(this, &UHealthBar::UpdateLifeStatus);
		UpdateLifeStatus(OwnerDamageHandler->GetOwner(), ELifeStatus::Invalid, OwnerDamageHandler->GetLifeStatus());
	}
}

void UHealthBar::UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth)
{
	//This function is called when health, max health, or absorb changes, so we just ignore the parameters and query them from the damage handler.
	if (!IsValid(OwnerDamageHandler) || OwnerDamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		return;
	}
	HealthBar->SetPercent(FMath::Clamp(OwnerDamageHandler->GetCurrentHealth() / FMath::Max(1.0f, OwnerDamageHandler->GetMaxHealth()), 0.0f, 1.0f));
	AbsorbBar->SetPercent(FMath::Clamp(OwnerDamageHandler->GetCurrentAbsorb() / FMath::Max(1.0f, OwnerDamageHandler->GetMaxHealth()), 0.0f, 1.0f));
	LeftText->SetText(FText::FromString(FString::FromInt(FMath::TruncToInt(OwnerDamageHandler->GetCurrentHealth()))));
	RightText->SetText(FText::FromString(FString::FromInt(FMath::TruncToInt(OwnerDamageHandler->GetMaxHealth()))));
}

void UHealthBar::UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (!IsValid(OwnerDamageHandler))
	{
		return;
	}
	switch (NewStatus)
	{
	case ELifeStatus::Alive :
		LeftText->SetVisibility(ESlateVisibility::HitTestInvisible);
		RightText->SetVisibility(ESlateVisibility::HitTestInvisible);
		MiddleText->SetText(FText::FromString("/"));
		UpdateHealth(OwnerDamageHandler->GetOwner(), 0.0f, 0.0f);
		break;
	default :
		HealthBar->SetPercent(0.0f);
		AbsorbBar->SetPercent(0.0f);
		LeftText->SetVisibility(ESlateVisibility::Hidden);
		RightText->SetVisibility(ESlateVisibility::Hidden);
		MiddleText->SetText(FText::FromString("Dead"));
		break;
	}
}