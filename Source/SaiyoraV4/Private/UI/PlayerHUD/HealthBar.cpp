﻿#include "PlayerHUD/HealthBar.h"
#include "DamageHandler.h"
#include "PlayerHUD.h"
#include "ProgressBar.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraPlayerCharacter.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"

void UHealthBar::NativeDestruct()
{
	Super::NativeDestruct();

	if (IsValid(OwnerDamageHandler))
	{
		OwnerDamageHandler->OnHealthChanged.RemoveDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnMaxHealthChanged.RemoveDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnAbsorbChanged.RemoveDynamic(this, &UHealthBar::UpdateHealth);
		OwnerDamageHandler->OnLifeStatusChanged.RemoveDynamic(this, &UHealthBar::UpdateLifeStatus);
	}
	if (IsValid(OwnerHUD))
	{
		OwnerHUD->OnExtraInfoToggled.RemoveDynamic(this, &UHealthBar::ToggleExtraInfo);
	}
}

void UHealthBar::InitHealthBar(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* OwningPlayer)
{
	if (!IsValid(OwningHUD) || !IsValid(OwningPlayer) || !OwningPlayer->Implements<USaiyoraCombatInterface>())
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
	OwnerHUD = OwningHUD;
	OwnerHUD->OnExtraInfoToggled.AddDynamic(this, &UHealthBar::ToggleExtraInfo);
	ToggleExtraInfo(OwnerHUD->IsExtraInfoToggled());
}

void UHealthBar::UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth)
{
	//This function is called when health, max health, or absorb changes, so we just ignore the parameters and query them from the damage handler.
	if (!IsValid(OwnerDamageHandler) || OwnerDamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		return;
	}
	//Set health and absorb progress bar percentages.
	const float HealthPercentage = FMath::Clamp(OwnerDamageHandler->GetCurrentHealth() / FMath::Max(1.0f, OwnerDamageHandler->GetMaxHealth()), 0.0f, 1.0f);
	HealthBar->SetPercent(HealthPercentage);
	AbsorbBar->SetPercent(FMath::Clamp(OwnerDamageHandler->GetCurrentAbsorb() / FMath::Max(1.0f, OwnerDamageHandler->GetMaxHealth()), 0.0f, 1.0f));
	//If extra info is toggled on, show health text as real numbers in the format Current / Max.
	if (bShowingExtraInfo)
	{
		if (LeftText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			LeftText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		LeftText->SetText(FText::FromString(FString::FromInt(FMath::TruncToInt(OwnerDamageHandler->GetCurrentHealth()))));
		MiddleText->SetText(FText::FromString("/"));
		if (RightText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			RightText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		RightText->SetText(FText::FromString(FString::FromInt(FMath::TruncToInt(OwnerDamageHandler->GetMaxHealth()))));
	}
	//If extra info is toggled off, show text as a single percentage in the middle of the bar.
	else
	{
		if (LeftText->GetVisibility() != ESlateVisibility::Hidden)
		{
			LeftText->SetVisibility(ESlateVisibility::Hidden);
		}
		MiddleText->SetText(FText::FromString(FString::FromInt(FMath::TruncToInt(HealthPercentage * 100)) + "%"));
		if (RightText->GetVisibility() != ESlateVisibility::Hidden)
		{
			RightText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
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
		UpdateHealth();
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

void UHealthBar::ToggleExtraInfo(const bool bShowExtraInfo)
{
	if (bShowExtraInfo != bShowingExtraInfo)
	{
		bShowingExtraInfo = bShowExtraInfo;
		UpdateHealth();
	}
}