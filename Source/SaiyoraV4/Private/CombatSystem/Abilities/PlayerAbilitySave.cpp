// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatSystem/Abilities/PlayerAbilitySave.h"
#include "PlayerAbilityHandler.h"

UPlayerAbilitySave::UPlayerAbilitySave()
{
	for (int32 i = 1; i <= UPlayerAbilityHandler::GetAbilitiesPerBar(); i++)
	{
		PlayerAbilityLoadout.AncientLoadout.Add(i, nullptr);
		PlayerAbilityLoadout.ModernLoadout.Add(i, nullptr);
		PlayerAbilityLoadout.HiddenLoadout.Add(i, nullptr);
	}
}

void UPlayerAbilitySave::AddUnlockedAbility(TSubclassOf<UCombatAbility> const NewAbility)
{
	UnlockedAbilities.Add(NewAbility);
}

void UPlayerAbilitySave::ModifyLoadout(EActionBarType const BarType, int32 const SlotNumber,
	TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (SlotNumber < 1 || SlotNumber > UPlayerAbilityHandler::GetAbilitiesPerBar())
	{
		return;
	}
	TMap<int32, TSubclassOf<UCombatAbility>>* Loadout = nullptr;
	switch (BarType)
	{
		case EActionBarType::None :
			break;
		case EActionBarType::Ancient :
			Loadout = &PlayerAbilityLoadout.AncientLoadout;
			break;
		case EActionBarType::Modern :
			Loadout = &PlayerAbilityLoadout.ModernLoadout;
			break;
		case EActionBarType::Hidden :
			Loadout = &PlayerAbilityLoadout.HiddenLoadout;
			break;
		default :
			break;
	}
	if (!Loadout)
	{
		return;
	}
	Loadout->Add(SlotNumber, AbilityClass);
}

void UPlayerAbilitySave::GetUnlockedAbilities(TSet<TSubclassOf<UCombatAbility>>& OutAbilities)
{
	OutAbilities = UnlockedAbilities;
}

void UPlayerAbilitySave::GetLastSavedLoadout(FPlayerAbilityLoadout& OutLoadout)
{
	OutLoadout = PlayerAbilityLoadout;
}
