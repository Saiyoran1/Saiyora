#include "AncientTalentWindow.h"
#include "AncientSpecialization.h"
#include "AncientTalent.h"
#include "CombatAbility.h"

void UAncientTalentWindow::NativeOnInitialized()
{
	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningPlayerPawn());
	if (IsValid(OwningPlayer) && IsValid(OwningPlayer->GetPlayerAbilityData()))
	{
		//Set up a layout for each ancient spec.
		for (const TSubclassOf<UAncientSpecialization> AncientSpecClass : OwningPlayer->GetPlayerAbilityData()->AncientSpecializations)
		{
			FAncientSpecLayout Layout;
			Layout.Spec = AncientSpecClass;
			const UAncientSpecialization* DefaultSpec = AncientSpecClass->GetDefaultObject<UAncientSpecialization>();
			if (IsValid(DefaultSpec))
			{
				TArray<FAncientTalentRow> TalentRows;
				DefaultSpec->GetTalentRows(TalentRows);
				const int32 NumAbilities = FMath::Min(TalentRows.Num(), OwningPlayer->GetPlayerAbilityData()->NumAncientAbilities);
				for (int i = 0; i < NumAbilities; i++)
				{
					Layout.Talents.Add(i, FAncientTalentChoice(TalentRows[i]));
				}
			}
			Layouts.Add(AncientSpecClass, Layout);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Ancient talent window failed to find player ability data asset!"));
	}
	//If we're in a dungeon, we should only enable talent/spec swapping during the "waiting to start" phase.
	GameStateRef = Cast<ADungeonGameState>(GetWorld()->GetGameState());
	if (IsValid(GameStateRef))
	{
		bEnabled = GameStateRef->GetDungeonPhase() == EDungeonPhase::WaitingToStart;
		GameStateRef->OnDungeonPhaseChanged.AddDynamic(this, &UAncientTalentWindow::OnDungeonPhaseChanged);
	}
	else
	{
		bEnabled = true;
	}
	//Call this late because the Blueprint logic called in OnInitialize depends on us having the player ref and layouts already set up.
	Super::NativeOnInitialized();
}

void UAncientTalentWindow::SelectSpec(const TSubclassOf<UAncientSpecialization> Spec)
{
	if (!bEnabled || !IsValid(Spec))
	{
		return;
	}
	CurrentSpec = Spec;
	PostChange();
}

void UAncientTalentWindow::UpdateTalentChoice(const TSubclassOf<UCombatAbility> BaseAbility,
                                              const TSubclassOf<UAncientTalent> TalentSelection)
{
	if (!bEnabled || !IsValid(CurrentSpec) || !IsValid(BaseAbility))
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
		if (Choice.Value.TalentRow.BaseAbilityClass == BaseAbility)
		{
			if (IsValid(TalentSelection))
			{
				if (Choice.Value.TalentRow.Talents.Contains(TalentSelection))
				{
					Choice.Value.CurrentSelection = TalentSelection;
				}
			}
			else
			{
				Choice.Value.CurrentSelection = nullptr;
			}
			break;
		}
	}
	PostChange();
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
	PostChange();
}

void UAncientTalentWindow::SaveLayout()
{
	LastSavedLayout = GetCurrentLayout();
	for (const TTuple<int32, FAncientTalentChoice>& TalentChoice : LastSavedLayout.Talents)
	{
		OwningPlayer->SetAbilityMapping(ESaiyoraPlane::Ancient, TalentChoice.Key, TalentChoice.Value.TalentRow.BaseAbilityClass);
	}
	OwningPlayer->ApplyNewAncientLayout(LastSavedLayout);
	PostChange();
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
		if (CurrentLayout->Talents[LastSavedRow.Key].TalentRow.BaseAbilityClass
			!= LastSavedRow.Value.TalentRow.BaseAbilityClass)
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
