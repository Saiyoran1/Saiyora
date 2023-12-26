﻿#include "ModernTalentWindow.h"
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
			const int32 NumSpecAbilities = OwningPlayer->GetPlayerAbilityData()->NumModernSpecAbilities;
			const int32 NumFlexAbilities = OwningPlayer->GetPlayerAbilityData()->NumModernFlexAbilities;
			for (int i = 0; i < NumSpecAbilities + NumFlexAbilities + 1; i++)
			{
				//First slot is always the weapon slot, which we can get from the modern spec CDO.
				if (i == 0)
				{
					const UModernSpecialization* DefaultSpec = ModernSpecClass->GetDefaultObject<UModernSpecialization>();
					TSubclassOf<UCombatAbility> WeaponClass = nullptr;
					if (DefaultSpec)
					{
						WeaponClass = DefaultSpec->GetWeaponClass();
					}
					Layout.Talents.Add(i, FModernTalentChoice(EModernSlotType::Weapon, WeaponClass));
				}
				//All other specs are either core spec slots or flex slots, but will not have an initial selection.
				else
				{
					Layout.Talents.Add(i, FModernTalentChoice(i <= NumSpecAbilities ? EModernSlotType::Spec : EModernSlotType::Flex));
				}
			}
			Layouts.Add(ModernSpecClass, Layout);
		}
	}

	//Call this late because the Blueprint logic called in OnInitialize depends on us having the player ref and layouts already set up.
	Super::NativeOnInitialized();
}

void UModernTalentWindow::SelectTalent(const int32 SlotNumber, const TSubclassOf<UCombatAbility> Talent)
{
	if (!IsValid(CurrentSpec) || !IsValid(Talent))
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
		const UModernSpecialization* DefaultSpec = CurrentSpec->GetDefaultObject<UModernSpecialization>();
		if (DefaultSpec)
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
}

void UModernTalentWindow::SwapTalentSlots(const int32 FirstSlot, const int32 SecondSlot)
{
	if (FirstSlot == SecondSlot || !IsValid(OwningPlayer) || !IsValid(OwningPlayer->GetPlayerAbilityData()))
	{
		return;
	}
	if (FirstSlot < 0 || FirstSlot > OwningPlayer->GetPlayerAbilityData()->NumModernSpecAbilities + OwningPlayer->GetPlayerAbilityData()->NumModernFlexAbilities)
	{
		return;
	}
	if (SecondSlot < 0 || SecondSlot > OwningPlayer->GetPlayerAbilityData()->NumModernSpecAbilities + OwningPlayer->GetPlayerAbilityData()->NumModernFlexAbilities)
	{
		return;
	}
	FModernSpecLayout* Layout = Layouts.Find(CurrentSpec);
	if (!Layout)
	{
		return;
	}
	if (!Layout->Talents.Contains(FirstSlot) || !Layout->Talents.Contains(SecondSlot))
	{
		return;
	}
	const FModernTalentChoice OriginalFirstSlot = Layout->Talents.FindRef(FirstSlot);
	const FModernTalentChoice* OriginalSecondSlot = Layout->Talents.Find(SecondSlot);
	Layout->Talents.Add(FirstSlot, *OriginalSecondSlot);
	Layout->Talents.Add(SecondSlot, OriginalFirstSlot);
}

void UModernTalentWindow::SaveLayout()
{
	LastSavedLayout = GetCurrentLayout();
	for (const TTuple<int32, FModernTalentChoice>& TalentChoice : LastSavedLayout.Talents)
	{
		OwningPlayer->SetAbilityMapping(ESaiyoraPlane::Modern, TalentChoice.Key, TalentChoice.Value.Selection);
	}
	OwningPlayer->ApplyNewModernLayout(LastSavedLayout);
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
