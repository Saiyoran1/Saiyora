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
		UpdateName(NAME_None);
	}
}

void UFloatingName::UpdateName(const FName InName, const EFaction OwnerFaction)
{
	if (InName == NAME_None)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
		const FString NameAsString = InName.ToString();
		if (NameAsString.Len() > MaxNameLength)
		{
			FString Shortened = NameAsString.Left(MaxNameLength - 3);
			Shortened += "...";
			NameText->SetText(FText::FromString(Shortened));
		}
		else
		{
			NameText->SetText(FText::FromString(NameAsString));
		}
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
