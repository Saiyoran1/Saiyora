#include "DungeonOverlay/BossRequirementDisplay.h"
#include "DungeonGameState.h"
#include "SaiyoraDevSettings.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"

void UBossRequirementDisplay::InitializeBossRequirement(const FString& BossDisplayName)
{
	if (IsValid(RequirementText))
	{
		RequirementText->SetText(FText::FromString(BossDisplayName));
		UpdateRequirementMet(false);
	}
}

void UBossRequirementDisplay::UpdateRequirementMet(const bool bMet)
{
	const ADungeonGameState* GameStateRef = GetWorld()->GetGameState<ADungeonGameState>();
	if (IsValid(RequirementText))
	{
		if (bMet)
		{
			if (IsValid(GameStateRef) && GameStateRef->IsDungeonDepleted())
			{
				RequirementText->SetColorAndOpacity(UI_DA->DungeonProgressFailureTextColor);
			}
			else
			{
				RequirementText->SetColorAndOpacity(UI_DA->DungeonProgressSuccessTextColor);
			}
		}
		else
		{
			RequirementText->SetColorAndOpacity(UI_DA->DungeonProgressDefaultTextColor);
		}
	}
	if (IsValid(TimestampText))
	{
		if (bMet && IsValid(GameStateRef))
		{
			TimestampText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(GameStateRef->GetServerWorldTimeSeconds())));
			if (GameStateRef->IsDungeonDepleted())
			{
				TimestampText->SetColorAndOpacity(UI_DA->DungeonProgressFailureTextColor);
			}
			else
			{
				TimestampText->SetColorAndOpacity(UI_DA->DungeonProgressSuccessTextColor);
			}
			TimestampText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TimestampText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}