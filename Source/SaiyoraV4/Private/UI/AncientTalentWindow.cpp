#include "AncientTalentWindow.h"
#include "AncientSpecialization.h"
#include "AncientTalent.h"
#include "CombatAbility.h"

void UAncientTalentWindow::NativeOnInitialized()
{
	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningPlayerPawn());
	if (IsValid(OwningPlayer) && IsValid(OwningPlayer->GetPlayerAbilityData()))
	{
		for (const TSubclassOf<UAncientSpecialization> AncientSpecClass : OwningPlayer->GetPlayerAbilityData()->AncientSpecializations)
		{
			Layouts.Add(AncientSpecClass, FAncientSpecLayout());
		}
	}

	Super::NativeOnInitialized();
}

void UAncientTalentWindow::UpdateTalentChoice(const TSubclassOf<UCombatAbility> BaseAbility,
	const TSubclassOf<UAncientTalent> TalentSelection)
{
	if (!IsValid(CurrentSpec) || !IsValid(BaseAbility) || !IsValid(TalentSelection))
	{
		return;
	}
	FAncientSpecLayout* Layout = Layouts.Find(CurrentSpec);
	if (!Layout)
	{
		return;
	}
	for (TTuple<int32, FAncientTalentChoice>& Choice : Layout->Talents)
	{
		if (Choice.Value.BaseAbility == BaseAbility)
		{
			if (Choice.Value.Talents.Contains(TalentSelection))
			{
				Choice.Value.CurrentSelection = TalentSelection;
			}
			break;
		}
	}
}

void UAncientTalentWindow::SwapTalentRowSlots(const int32 FirstRow, const int32 SecondRow)
{
	if (FirstRow == SecondRow || !IsValid(OwningPlayer) || !IsValid(OwningPlayer->GetPlayerAbilityData()))
	{
		return;
	}
	if (FirstRow < 0 || FirstRow > OwningPlayer->GetPlayerAbilityData()->NumAncientAbilities - 1)
	{
		return;
	}
	if (SecondRow < 0 || SecondRow > OwningPlayer->GetPlayerAbilityData()->NumAncientAbilities - 1)
	{
		return;
	}
	FAncientSpecLayout* Layout = Layouts.Find(CurrentSpec);
	if (!Layout)
	{
		return;
	}
	if (!Layout->Talents.Contains(FirstRow) || !Layout->Talents.Contains(SecondRow))
	{
		return;
	}
	const FAncientTalentChoice OriginalFirstRow = Layout->Talents.FindRef(FirstRow);
	const FAncientTalentChoice* OriginalSecondRow = Layout->Talents.Find(SecondRow);
	Layout->Talents.Add(FirstRow, *OriginalSecondRow);
	Layout->Talents.Add(SecondRow, OriginalFirstRow);
}

bool UAncientTalentWindow::IsLayoutDirty() const
{
	if (!IsValid(CurrentSpec) || !IsValid(LastSavedLayout.Spec))
	{
		return true;
	}
	if (CurrentSpec != LastSavedLayout.Spec)
	{
		return true;
	}
	const FAncientSpecLayout* CurrentLayout = Layouts.Find(CurrentSpec);
	if (!CurrentLayout)
	{
		return true;
	}
	for (const TTuple<int32, FAncientTalentChoice>& LastSavedRow : LastSavedLayout.Talents)
	{
		if (!CurrentLayout->Talents.Contains(LastSavedRow.Key))
		{
			return true;
		}
		if (CurrentLayout->Talents[LastSavedRow.Key].BaseAbility
			!= LastSavedRow.Value.BaseAbility)
		{
			return true;
		}
		if (CurrentLayout->Talents[LastSavedRow.Key].CurrentSelection
			!= LastSavedRow.Value.CurrentSelection)
		{
			return true;
		}
	}
	return false;
}

void UAncientTalentWindow::SaveLayout()
{
	LastSavedLayout = GetCurrentLayout();
	for (const TTuple<int32, FAncientTalentChoice>& TalentChoice : LastSavedLayout.Talents)
	{
		OwningPlayer->SetAbilityMapping(ESaiyoraPlane::Ancient, TalentChoice.Key, TalentChoice.Value.BaseAbility);
	}
	OwningPlayer->ApplyNewAncientLayout(LastSavedLayout);
}
