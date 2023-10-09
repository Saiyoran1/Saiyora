#include "FloatingName.h"
#include "CombatStatusComponent.h"
#include "TextBlock.h"

void UFloatingName::Init(UCombatStatusComponent* CombatStatusComponent)
{
	if (IsValid(CombatStatusComponent))
	{
		OwnerCombatStatus = CombatStatusComponent;
		OwnerCombatStatus->OnNameChanged.AddDynamic(this, &UFloatingName::UpdateName);
		UpdateName(NAME_None, OwnerCombatStatus->GetCombatName());
	}
	else
	{
		UpdateName(NAME_None, NAME_None);
	}
}

void UFloatingName::UpdateName(const FName PreviousName, const FName NewName)
{
	if (NewName == NAME_None)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
		const FString NameAsString = NewName.ToString();
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
		switch (OwnerCombatStatus->GetCurrentFaction())
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
