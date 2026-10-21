#include "DungeonOverlay/DungeonDisplay.h"
#include "DungeonGameState.h"
#include "Image.h"
#include "PlayerHUD.h"
#include "ProgressBar.h"
#include "SaiyoraDevSettings.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"
#include "VerticalBox.h"
#include "DungeonOverlay/BossRequirementDisplay.h"

void UDungeonDisplay::InitDungeonDisplay(UPlayerHUD* OwningHUD)
{
	if (!IsValid(OwningHUD))
	{
		return;
	}
	OwnerHUD = OwningHUD;
	GameStateRef = Cast<ADungeonGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameStateRef))
	{
		return;
	}
	if (IsValid(NameText))
	{
		NameText->SetText(FText::FromString(GameStateRef->GetDungeonName()));
	}
	if (IsValid(BossReqWidgetClass))
	{
		TMap<FGameplayTag, FString> BossRequirements;
		GameStateRef->GetBossRequirements(BossRequirements);
		for (const TTuple<FGameplayTag, FString>& Requirement : BossRequirements)
		{
			UBossRequirementDisplay* BossReqWidget = CreateWidget<UBossRequirementDisplay>(this, BossReqWidgetClass);
			if (IsValid(BossReqWidget))
			{
				BossReqWidget->InitializeBossRequirement(Requirement.Value);
				if (GameStateRef->IsBossKilled(Requirement.Key))
				{
					BossReqWidget->UpdateRequirementMet(true);
				}
				BossReqWidgets.Add(Requirement.Key, BossReqWidget);
				BossBox->AddChildToVerticalBox(BossReqWidget);
				//TODO: Adjust vert box slot if needed.
			}
		}
		GameStateRef->OnBossKilled.AddDynamic(this, &UDungeonDisplay::OnBossKilled);
	}
	OnDungeonPhaseChanged(EDungeonPhase::None, GameStateRef->GetDungeonPhase());
	GameStateRef->OnDungeonPhaseChanged.AddDynamic(this, &UDungeonDisplay::OnDungeonPhaseChanged);
	OnDeathCountChanged(GameStateRef->GetCurrentDeathCount(), GameStateRef->GetCurrentDeathPenaltyTime());
	GameStateRef->OnDeathCountChanged.AddDynamic(this, &UDungeonDisplay::OnDeathCountChanged);
	OnKillCountChanged(GameStateRef->GetCurrentKillCount(), GameStateRef->GetKillCountRequirement());
	GameStateRef->OnKillCountChanged.AddDynamic(this, &UDungeonDisplay::OnKillCountChanged);
	if (GameStateRef->IsDungeonDepleted())
	{
		OnDungeonDepleted();
	}
	else
	{
		GameStateRef->OnDungeonDepleted.AddDynamic(this, &UDungeonDisplay::OnDungeonDepleted);
	}
}

void UDungeonDisplay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(GameStateRef) || GameStateRef->GetDungeonPhase() != EDungeonPhase::InProgress)
	{
		return;
	}
	if (IsValid(TimeText))
	{
		TimeText->SetText(FText::FromString(FString::Printf(L"%s / %s",
			*UUIFunctionLibrary::GetTimeDisplayString(GameStateRef->GetElapsedDungeonTime()),
			*UUIFunctionLibrary::GetTimeDisplayString(GameStateRef->GetDungeonTimeLimit()))));
	}
	if (!GameStateRef->IsDungeonDepleted() && IsValid(TimeProgress))
	{
		const float Percent = FMath::Clamp(GameStateRef->GetElapsedDungeonTime() / GameStateRef->GetDungeonTimeLimit(), 0.0f, 1.0f);
		TimeProgress->SetPercent(Percent);
		TimeProgress->SetFillColorAndOpacity(FMath::Lerp(UI_DA->DungeonProgressStartBarColor, UI_DA->DungeonProgressEndBarColor, Percent));
	}
}

void UDungeonDisplay::OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase)
{
	switch (NewPhase)
	{
	case EDungeonPhase::WaitingToStart :
	case EDungeonPhase::Countdown :
		if (IsValid(GameStateRef) && IsValid(TimeText))
		{
			TimeText->SetText(FText::FromString(FString::Printf(L"Time Limit: %s", *UUIFunctionLibrary::GetTimeDisplayString(GameStateRef->GetDungeonTimeLimit()))));
		}
		if (IsValid(DeathCountImage))
		{
			DeathCountImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (IsValid(DeathCountText))
		{
			DeathCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
		break;
	case EDungeonPhase::Completed :
		if (!GameStateRef->IsDungeonDepleted())
		{
			if (IsValid(TimeProgress))
			{
				TimeProgress->SetFillColorAndOpacity(UI_DA->SuccessProgressColor);
			}
		}
		break;
	default :
		break;
	}
}

void UDungeonDisplay::OnDeathCountChanged(const int32 DeathCount, const float PenaltyTime)
{
	if (IsValid(DeathCountText))
	{
		DeathCountText->SetText(FText::FromString(FString::Printf(L"%i (+%s)", DeathCount, *UUIFunctionLibrary::GetTimeDisplayString(PenaltyTime))));
		if (DeathCount > 0)
		{
			DeathCountText->SetColorAndOpacity(UI_DA->DungeonProgressFailureTextColor);
		}
		else
		{
			DeathCountText->SetColorAndOpacity(UI_DA->DungeonProgressDefaultTextColor);
		}
	}
}

void UDungeonDisplay::OnKillCountChanged(const int32 KillCount, const int32 MaxCount)
{
	if (IsValid(KillCountText))
	{
		KillCountText->SetText(FText::FromString(FString::Printf(L"Kill Count: %i / %i", KillCount, MaxCount)));
		if (KillCount >= MaxCount)
		{
			KillCountText->SetColorAndOpacity(UI_DA->DungeonProgressSuccessTextColor);
		}
		else
		{
			KillCountText->SetColorAndOpacity(UI_DA->DungeonProgressDefaultTextColor);
		}
	}
	if (IsValid(KillCountProgress))
	{
		const float Percent = FMath::Clamp(static_cast<float>(KillCount) / MaxCount, 0.0f, 1.0f);
		KillCountProgress->SetPercent(Percent);
		if (KillCount < MaxCount)
		{
			KillCountProgress->SetFillColorAndOpacity(FMath::Lerp(UI_DA->DungeonProgressEndBarColor, UI_DA->DungeonProgressStartBarColor, Percent));
		}
		else
		{
			KillCountProgress->SetFillColorAndOpacity(UI_DA->SuccessProgressColor);
		}
	}
}

void UDungeonDisplay::OnBossKilled(const FGameplayTag BossTag, const FString& BossName)
{
	UBossRequirementDisplay* BossReqWidget = BossReqWidgets.FindRef(BossTag);
	if (IsValid(BossReqWidget))
	{
		BossReqWidget->UpdateRequirementMet(true);
	}
}

void UDungeonDisplay::OnDungeonDepleted()
{
	if (IsValid(TimeText))
	{
		TimeText->SetColorAndOpacity(UI_DA->DungeonProgressFailureTextColor);
	}
	if (IsValid(TimeProgress))
	{
		TimeProgress->SetFillColorAndOpacity(UI_DA->FailureProgressColor);
	}
}