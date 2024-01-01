#include "ModernSpecialization.h"
#include "AbilityComponent.h"
#include "ResourceHandler.h"
#include "UnrealNetwork.h"
#include "SaiyoraPlayerCharacter.h"

void UModernSpecialization::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UModernSpecialization, Loadout);
}

void UModernSpecialization::InitializeSpecialization(ASaiyoraPlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	OwningPlayer = PlayerCharacter;
	if (OwningPlayer->GetLocalRole() == ROLE_Authority)
	{
		if (IsValid(ResourceClass))
		{
			UResourceHandler* OwnerResourceComp = ISaiyoraCombatInterface::Execute_GetResourceHandler(OwningPlayer);
			if (IsValid(OwnerResourceComp))
			{
				OwnerResourceComp->AddNewResource(ResourceClass, ResourceInitInfo);
			}
		}
		UAbilityComponent* OwnerAbilityCompRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
		if (IsValid(OwnerAbilityCompRef))
		{
			OwnerAbilityCompRef->AddNewAbility(WeaponClass);
		}
		//Fill the loadout array with the correct slot types and null choices for non-weapon abilities.
		Loadout.Add(FModernTalentChoice(EModernSlotType::Weapon, WeaponClass));
		for (int i = 0; i < OwningPlayer->GetPlayerAbilityData()->NumModernSpecAbilities; i++)
		{
			Loadout.Add(FModernTalentChoice(EModernSlotType::Spec));
		}
		for (int i = 0; i < OwningPlayer->GetPlayerAbilityData()->NumModernFlexAbilities; i++)
		{
			Loadout.Add(FModernTalentChoice(EModernSlotType::Flex));
		}
	}
	bInitialized = true;
	OnLearn();
}

void UModernSpecialization::UnlearnSpec()
{
	if (OwningPlayer->GetLocalRole() == ROLE_Authority)
	{
		if (IsValid(ResourceClass))
		{
			UResourceHandler* OwnerResourceHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(OwningPlayer);
			if (IsValid(OwnerResourceHandler))
			{
				OwnerResourceHandler->RemoveResource(ResourceClass);
			}
		}
		UAbilityComponent* OwnerAbilityCompRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
		if (IsValid(OwnerAbilityCompRef))
		{
			for (const FModernTalentChoice& TalentChoice : Loadout)
			{
				OwnerAbilityCompRef->RemoveAbility(TalentChoice.Selection);
			}
		}
	}
	OnUnlearn();
}

void UModernSpecialization::SelectModernAbilities(const TArray<FModernTalentChoice>& Selections)
{
	if (!IsValid(OwningPlayer) || !OwningPlayer->HasAuthority())
	{
		return;
	}
	const UPlayerAbilityData* PlayerAbilityData = OwningPlayer->GetPlayerAbilityData();
	if (!IsValid(PlayerAbilityData))
	{
		return;
	}
	UAbilityComponent* OwningPlayerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
	if (!IsValid(OwningPlayerAbilityComp))
	{
		return;
	}
	
	TArray<FModernTalentChoice> DuplicateSelections;
	TArray<FModernTalentChoice> NewSelections;
	
	for (const FModernTalentChoice& Selection : Selections)
	{
		if (Selection.Selection == nullptr)
		{
			continue;
		}
		//Check if we already know this ability.
		if (Loadout.Contains(Selection))
		{
			DuplicateSelections.Add(Selection);
			continue;
		}
		switch (Selection.SlotType)
		{
		case EModernSlotType::Spec :
			{
				//Check the spec-specific pool.
				if (SpecAbilityPool.Contains(Selection.Selection))
				{
					NewSelections.Add(Selection);
				}
			}
			break;
		case EModernSlotType::Flex :
			{
				//Check the generic modern ability pool.
				if (PlayerAbilityData->ModernAbilityPool.Contains(Selection.Selection))
				{
					NewSelections.Add(Selection);
				}
			}
			break;
		default :
			//Notably, the weapon ability isn't even considered, as it should never change.
			continue;
		}
	}
	
	//Iterate over our loadout and change out abilities that weren't chosen in the new loadout.
	for (FModernTalentChoice& Choice : Loadout)
	{
		//Don't replace the weapon slot, it shouldn't ever change.
		if (Choice.SlotType == EModernSlotType::Weapon)
		{
			continue;
		}
		//If this selection was in the old loadout and the new loadout, we can skip it.
		if (DuplicateSelections.Contains(Choice))
		{
			continue;
		}
		//Iterate over our new selections and find one that matches our slot type.
		bool bUpdatedSlot = false;
		for (int i = 0; i < NewSelections.Num(); i++)
		{
			if (NewSelections[i].SlotType == Choice.SlotType)
			{
				//Once we've found a matching slot type, unlearn the current choice and learn the new choice.
				OwningPlayerAbilityComp->RemoveAbility(Choice.Selection);
				const UCombatAbility* NewAbilityInstance = OwningPlayerAbilityComp->AddNewAbility(NewSelections[i].Selection);
				if (IsValid(NewAbilityInstance))
				{
					Choice.Selection = NewSelections[i].Selection;
				}
				else
				{
					Choice.Selection = nullptr;
				}
				bUpdatedSlot = true;
				NewSelections.RemoveAt(i);
				break;
			}
		}
		if (!bUpdatedSlot)
		{
			//Nothing in the new loadout matches our slot type, or there aren't any more selections, so unlearn this.
			OwningPlayerAbilityComp->RemoveAbility(Choice.Selection);
			Choice.Selection = nullptr;
		}
	}
}