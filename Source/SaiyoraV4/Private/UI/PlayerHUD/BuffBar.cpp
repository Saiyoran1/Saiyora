#include "BuffBar.h"
#include "Buff.h"
#include "Image.h"
#include "PlayerHUD.h"
#include "ProgressBar.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"

void UBuffBar::Init(UPlayerHUD* OwnerHUD)
{
	if (!IsValid(OwnerHUD))
	{
		return;
	}
	OwningHUD = OwnerHUD;
	OwningHUD->OnExtraInfoToggled.AddDynamic(this, &UBuffBar::ToggleExtraInfo);
	ToggleExtraInfo(OwningHUD->IsExtraInfoToggled());
}

void UBuffBar::SetBuff(UBuff* NewBuff)
{
	if (NewBuff == AssignedBuff)
	{
		return;
	}
	if (IsValid(AssignedBuff))
	{
		AssignedBuff->OnUpdated.RemoveDynamic(this, &UBuffBar::OnBuffUpdated);
	}
	AssignedBuff = NewBuff;
	if (!IsValid(AssignedBuff))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	IconImage->SetBrushFromTexture(AssignedBuff->GetBuffIcon());
	NameText->SetText(FText::FromName(AssignedBuff->GetBuffName()));
	DescriptionText->SetText(AssignedBuff->GetBuffDescription());
	
	AssignedBuff->OnUpdated.AddDynamic(this, &UBuffBar::OnBuffUpdated);
	const int Stacks = AssignedBuff->GetCurrentStacks();
	StackText->SetText(FText::FromString(Stacks > 1 ? FString::FromInt(Stacks) : ""));

	DurationBar->SetFillColorAndOpacity(AssignedBuff->GetBuffProgressColor());
	if (AssignedBuff->HasFiniteDuration())
	{
		DurationBar->SetPercent(
			FMath::Clamp(AssignedBuff->GetRemainingTime() / FMath::Max(1.0f, AssignedBuff->GetExpirationTime() - AssignedBuff->GetLastRefreshTime()),
				0.0f, 1.0f));
	}
	else
	{
		DurationBar->SetPercent(1.0f);
	}

	DurationText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(AssignedBuff->GetRemainingTime())));

	ToggleExtraInfo(bShowingExtraInfo);
}

void UBuffBar::ToggleExtraInfo(const bool bShowExtraInfo)
{
	bShowingExtraInfo = bShowExtraInfo;
	if (!IsValid(AssignedBuff))
	{
		return;
	}
	if (bShowingExtraInfo)
	{
		if (NameText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			NameText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (DurationText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			DurationText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (DescriptionText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			DescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	else
	{
		if (NameText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (DurationText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			DurationText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (DescriptionText->GetVisibility() != ESlateVisibility::Collapsed)
		{
			DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBuffBar::OnBuffUpdated(const FBuffApplyEvent& ApplyEvent)
{
	const int Stacks = AssignedBuff->GetCurrentStacks();
	StackText->SetText(FText::FromString(Stacks > 1 ? FString::FromInt(Stacks) : ""));
}

void UBuffBar::NativeDestruct()
{
	Super::NativeDestruct();

	if (IsValid(AssignedBuff))
	{
		AssignedBuff->OnUpdated.RemoveDynamic(this, &UBuffBar::OnBuffUpdated);
	}
	if (IsValid(OwningHUD))
	{
		OwningHUD->OnExtraInfoToggled.RemoveDynamic(this, &UBuffBar::ToggleExtraInfo);
	}
}

void UBuffBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(AssignedBuff))
	{
		return;
	}

	if (AssignedBuff->HasFiniteDuration())
	{
		DurationBar->SetPercent(
			FMath::Clamp(AssignedBuff->GetRemainingTime() / FMath::Max(1.0f, AssignedBuff->GetExpirationTime() - AssignedBuff->GetLastRefreshTime()),
				0.0f, 1.0f));
		DurationText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(AssignedBuff->GetRemainingTime())));
	}
}