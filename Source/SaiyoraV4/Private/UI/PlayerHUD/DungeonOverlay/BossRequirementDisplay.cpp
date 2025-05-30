#include "DungeonOverlay/BossRequirementDisplay.h"
#include "DungeonGameState.h"
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
	const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
	const bool bUpdateColors = IsValid(UIDataAsset);
	const ADungeonGameState* GameStateRef = GetWorld()->GetGameState<ADungeonGameState>();
	if (IsValid(RequirementText) && bUpdateColors)
	{
		if (bMet)
		{
			if (IsValid(GameStateRef) && GameStateRef->IsDungeonDepleted())
			{
				RequirementText->SetColorAndOpacity(UIDataAsset->DungeonProgressFailureTextColor);
			}
			else
			{
				RequirementText->SetColorAndOpacity(UIDataAsset->DungeonProgressSuccessTextColor);
			}
		}
		else
		{
			RequirementText->SetColorAndOpacity(UIDataAsset->DungeonProgressDefaultTextColor);
		}
	}
	if (IsValid(TimestampText))
	{
		if (bMet && IsValid(GameStateRef))
		{
			TimestampText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(GameStateRef->GetServerWorldTimeSeconds())));
			if (bUpdateColors)
			{
				if (GameStateRef->IsDungeonDepleted())
				{
					TimestampText->SetColorAndOpacity(UIDataAsset->DungeonProgressFailureTextColor);
				}
				else
				{
					TimestampText->SetColorAndOpacity(UIDataAsset->DungeonProgressSuccessTextColor);
				}
			}
			TimestampText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TimestampText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}