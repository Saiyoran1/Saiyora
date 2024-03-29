﻿#include "ModernTalentWindow.h"

#include "DungeonGameState.h"
#include "SaiyoraPlayerCharacter.h"

void UModernTalentWindow::NativeOnInitialized()
{
	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningPlayerPawn());
	if (IsValid(OwningPlayer) && IsValid(OwningPlayer->GetPlayerAbilityData()))
	{
		//Set up a layout for each modern spec.
		for (const TSubclassOf<UModernSpecialization> ModernSpecClass : OwningPlayer->GetPlayerAbilityData()->ModernSpecializations)
		{
			FModernSpecLayout Layout;
			Layout.Spec = ModernSpecClass;
			const UModernSpecialization* DefaultSpec = ModernSpecClass->GetDefaultObject<UModernSpecialization>();
			const int32 NumSpecAbilities = OwningPlayer->GetPlayerAbilityData()->NumModernSpecAbilities;
			const int32 NumFlexAbilities = OwningPlayer->GetPlayerAbilityData()->NumModernFlexAbilities;
			int32 SpecSlotIndex = 0;
			int32 FlexSlotIndex = 0;
			for (int i = 0; i < NumSpecAbilities + NumFlexAbilities + 1; i++)
			{
				//First slot is always the weapon slot, which we can get from the modern spec CDO.
				if (i == 0)
				{
					TSubclassOf<UCombatAbility> WeaponClass = nullptr;
					if (DefaultSpec)
					{
						WeaponClass = DefaultSpec->GetWeaponClass();
					}
					Layout.Talents.Add(i, FModernTalentChoice(EModernSlotType::Weapon, WeaponClass));
				}
				//All other specs are either core spec slots or flex slots, and we'll attempt to fill an initial selection.
				else if (i <= NumSpecAbilities)
				{
					//Spec slots we'll just get the first few abilities from the spec's pool, if they exist.
					TSubclassOf<UCombatAbility> SpecAbilityClass = nullptr;
					if (DefaultSpec)
					{
						TArray<TSubclassOf<UCombatAbility>> SpecAbilities;
						DefaultSpec->GetSpecAbilities(SpecAbilities);
						if (SpecSlotIndex <= SpecAbilities.Num() - 1)
						{
							SpecAbilityClass = SpecAbilities[SpecSlotIndex];
							SpecSlotIndex++;
						}
					}
					Layout.Talents.Add(i, FModernTalentChoice(EModernSlotType::Spec, SpecAbilityClass));
				}
				else
				{
					//Flex slots we'll just get the first few abilities from the generic pool, if they exist.
					TSubclassOf<UCombatAbility> FlexAbilityClass = nullptr;
					if (DefaultSpec)
					{
						if (FlexSlotIndex <= OwningPlayer->GetPlayerAbilityData()->ModernAbilityPool.Num() - 1)
						{
							FlexAbilityClass = OwningPlayer->GetPlayerAbilityData()->ModernAbilityPool[FlexSlotIndex];
							FlexSlotIndex++;
						}
					}
					Layout.Talents.Add(i, FModernTalentChoice(EModernSlotType::Flex, FlexAbilityClass));
				}
			}
			Layouts.Add(ModernSpecClass, Layout);
			//If this is the first spec we are generating a layout for, go ahead and set our current spec.
			if (!IsValid(CurrentSpec))
			{
				CurrentSpec = ModernSpecClass;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Modern talent window failed to find player ability data asset!"));
	}
	//If we're in a dungeon, we should only enable talent/spec swapping during the "waiting to start" phase.
	GameStateRef = Cast<ADungeonGameState>(GetWorld()->GetGameState());
	if (IsValid(GameStateRef))
	{
		bEnabled = GameStateRef->GetDungeonPhase() == EDungeonPhase::WaitingToStart;
		GameStateRef->OnDungeonPhaseChanged.AddDynamic(this, &UModernTalentWindow::OnDungeonPhaseChanged);
	}
	else
	{
		bEnabled = true;
	}
	//Call this late because the Blueprint logic called in OnInitialize depends on us having the player ref and layouts already set up.
	Super::NativeOnInitialized();
}

void UModernTalentWindow::SelectSpec(const TSubclassOf<UModernSpecialization> Spec)
{
	if (!bEnabled || !IsValid(Spec))
	{
		return;
	}
	CurrentSpec = Spec;
	PostChange();
}

void UModernTalentWindow::SelectTalent(const int32 SlotNumber, const TSubclassOf<UCombatAbility> Talent)
{
	if (!bEnabled || !IsValid(CurrentSpec) || !IsValid(Talent))
	{
		return;
	}
	FModernSpecLayout* CurrentLayout = Layouts.Find(CurrentSpec);
	if (!CurrentLayout)
	{
		return;
	}
	FModernTalentChoice* TalentChoice = CurrentLayout->Talents.Find(SlotNumber);
	//If we didn't find a talent choice for that slot, or the slot was a weapon slot (or none slot), we can't change it.
	if (!TalentChoice || TalentChoice->SlotType == EModernSlotType::None || TalentChoice->SlotType == EModernSlotType::Weapon)
	{
		return;
	}
	TArray<TSubclassOf<UCombatAbility>> ValidAbilities;
	//If this is a spec slot, the valid abilities we can swap to are determined by the modern spec's core ability pool.
	if (TalentChoice->SlotType == EModernSlotType::Spec)
	{
		if (const UModernSpecialization* DefaultSpec = CurrentSpec->GetDefaultObject<UModernSpecialization>())
		{
			DefaultSpec->GetSpecAbilities(ValidAbilities);
		}
	}
	//Otherwise, this is a flex slot, which pulls from the general modern ability pool for all players.
	else
	{
		ValidAbilities = OwningPlayer->GetPlayerAbilityData()->ModernAbilityPool;
	}
	if (!ValidAbilities.Contains(Talent))
	{
		return;
	}
	TalentChoice->Selection = Talent;
	//Iterate over all layout slots here, to clear selections for other slots that might have selected this talent.
	for (TTuple<int32, FModernTalentChoice>& Talents : CurrentLayout->Talents)
	{
		//Make sure to exclude the slot we just set.
		if (Talents.Key != SlotNumber && Talents.Value.Selection == Talent)
		{
			Talents.Value.Selection = nullptr;
		}
	}
	PostChange();
}

void UModernTalentWindow::SwapTalentSlots(const int32 FirstSlot, const int32 SecondSlot)
{
	if (FirstSlot == SecondSlot)
	{
		return;
	}
	FModernSpecLayout* Layout = Layouts.Find(CurrentSpec);
	if (!Layout)
	{
		return;
	}
	//Check our layout to find out if either slot index is invalid.
	if (!Layout->Talents.Contains(FirstSlot) || !Layout->Talents.Contains(SecondSlot))
	{
		return;
	}
	const FModernTalentChoice OriginalFirstSlot = Layout->Talents.FindRef(FirstSlot);
	const FModernTalentChoice* OriginalSecondSlot = Layout->Talents.Find(SecondSlot);
	Layout->Talents.Add(FirstSlot, *OriginalSecondSlot);
	Layout->Talents.Add(SecondSlot, OriginalFirstSlot);
	PostChange();
}

void UModernTalentWindow::SaveLayout()
{
	LastSavedLayout = GetCurrentLayout();
	//Locally set keybinds for the layout.
	for (const TTuple<int32, FModernTalentChoice>& TalentChoice : LastSavedLayout.Talents)
	{
		OwningPlayer->SetAbilityMapping(ESaiyoraPlane::Modern, TalentChoice.Key, TalentChoice.Value.Selection);
	}
	//Actually try to learn the new abilities.
	OwningPlayer->ApplyNewModernLayout(LastSavedLayout);
	PostChange();
}

bool UModernTalentWindow::IsLayoutDirty() const
{
	if (!IsValid(CurrentSpec) || !IsValid(LastSavedLayout.Spec))
	{
		return true;
	}
	if (CurrentSpec != LastSavedLayout.Spec)
	{
		return true;
	}
	const FModernSpecLayout* CurrentLayout = Layouts.Find(CurrentSpec);
	if (!CurrentLayout)
	{
		return true;
	}
	for (const TTuple<int32, FModernTalentChoice>& LastSavedTalent : LastSavedLayout.Talents)
	{
		if (!CurrentLayout->Talents.Contains(LastSavedTalent.Key))
		{
			return true;
		}
		if (CurrentLayout->Talents[LastSavedTalent.Key].Selection
			!= LastSavedTalent.Value.Selection)
		{
			return true;
		}
	}
	return false;
}
