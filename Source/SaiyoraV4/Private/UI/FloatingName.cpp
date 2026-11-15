#include "FloatingName.h"
#include "CombatStatusComponent.h"
#include "TextBlock.h"

void UFloatingName::Init(const UCombatStatusComponent* CombatStatusComponent)
{
	if (IsValid(CombatStatusComponent))
	{
		UpdateName(CombatStatusComponent->GetCombatName(), CombatStatusComponent->GetCurrentFaction());
	}
	else
	{
		UpdateName(FText());
	}
}

void UFloatingName::UpdateName(const FText& InName, const EFaction OwnerFaction)
{
	if (InName.IsEmptyOrWhitespace())
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
		NameText->SetText(InName);
		switch (OwnerFaction)
		{
		case EFaction::Friendly :
			NameText->SetColorAndOpacity(FColor::Green);
			break;
		case EFaction::Enemy :
			NameText->SetColorAndOpacity(FColor::Red);
			break;
		case EFaction::Neutral :
			NameText->SetColorAndOpacity(FColor::Yellow);
			break;
		default :
			NameText->SetColorAndOpacity(FColor::Red);
			break;
		}
	}
}
