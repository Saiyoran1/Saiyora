#include "Specialization/AncientSpecialization.h"
#include "AbilityComponent.h"
#include "CombatAbility.h"
#include "ResourceHandler.h"
#include "UnrealNetwork.h"
#include "Specialization/AncientTalent.h"
#include "CoreClasses/SaiyoraPlayerCharacter.h"

void UAncientSpecialization::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAncientSpecialization, Loadout);
}

void UAncientSpecialization::InitializeSpecialization(ASaiyoraPlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	OwningPlayer = PlayerCharacter;
	Loadout.OwningSpecialization = this;
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
		UAbilityComponent* OwnerAbilityComp = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
		if (IsValid(OwnerAbilityComp))
		{
			const int32 NumAbilities = FMath::Min(OwningPlayer->GetPlayerAbilityData()->NumAncientAbilities, TalentRows.Num());
			for (int i = 0; i < NumAbilities; i++)
			{
				Loadout.MarkItemDirty(Loadout.Items.Add_GetRef(FAncientTalentChoice(TalentRows[i])));
			}
			for (const FAncientTalentChoice& TalentChoice : Loadout.Items)
			{
				OwnerAbilityComp->AddNewAbility(TalentChoice.TalentRow.BaseAbilityClass);
			}
		}
	}
	else
	{
		//Update all talent instances to reflect replicated class choices.
		for (FAncientTalentChoice& TalentChoice : Loadout.Items)
		{
			if (IsValid(TalentChoice.CurrentSelection))
			{
				if (!IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, TalentChoice.CurrentSelection);
					if (IsValid(TalentChoice.ActiveTalent))
					{
						TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.TalentRow.BaseAbilityClass);
					}
				}
				else if (TalentChoice.ActiveTalent->GetClass() != TalentChoice.CurrentSelection)
				{
					TalentChoice.ActiveTalent->UnselectTalent();
					TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, TalentChoice.CurrentSelection);
					if (IsValid(TalentChoice.ActiveTalent))
					{
						TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.TalentRow.BaseAbilityClass);
					}
				}
			}
			else if (IsValid(TalentChoice.ActiveTalent))
			{
				TalentChoice.ActiveTalent->UnselectTalent();
				TalentChoice.ActiveTalent = nullptr;
			}
		}
	}
	bInitialized = true;
	OnLearn();
}

void UAncientSpecialization::UnlearnSpec()
{
	for (FAncientTalentChoice& TalentChoice : Loadout.Items)
	{
		if (IsValid(TalentChoice.ActiveTalent))
		{
			TalentChoice.ActiveTalent->UnselectTalent();
			TalentChoice.ActiveTalent = nullptr;
			TalentChoice.CurrentSelection = nullptr;
		}
	}
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
		UAbilityComponent* OwnerAbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
		if (IsValid(OwnerAbilityComponent))
		{
			for (const FAncientTalentChoice& TalentChoice : Loadout.Items)
			{
				if (IsValid(TalentChoice.TalentRow.BaseAbilityClass))
				{
					OwnerAbilityComponent->RemoveAbility(TalentChoice.TalentRow.BaseAbilityClass);
				}
			}
		}
	}
	OnUnlearn();
}

void UAncientSpecialization::SelectAncientTalent(const FAncientTalentSelection& NewSelection)
{
	if (!GetOwningPlayer()->HasAuthority() || !IsValid(NewSelection.BaseAbility))
	{
		return;
	}
	for (FAncientTalentChoice& TalentChoice : Loadout.Items)
	{
		if (TalentChoice.TalentRow.BaseAbilityClass == NewSelection.BaseAbility)
		{
			//If we currently have an active talent.
			if (IsValid(TalentChoice.CurrentSelection))
			{
				//If this talent is the same as the current selection, return.
				if (TalentChoice.CurrentSelection == NewSelection.Selection)
				{
					return;
				}
				const TSubclassOf<UAncientTalent> PreviousTalent = TalentChoice.CurrentSelection;
				//Unlearn the previous talent.
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->UnselectTalent();
				}
				TalentChoice.CurrentSelection = NewSelection.Selection;
				//If not using base ability, learn the new talent.
				if (IsValid(TalentChoice.CurrentSelection))
				{
					TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, NewSelection.Selection);
					if (IsValid(TalentChoice.ActiveTalent))
					{
						TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.TalentRow.BaseAbilityClass);
					}
				}
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(TalentChoice.TalentRow.BaseAbilityClass, PreviousTalent, TalentChoice.CurrentSelection);
			}
			//If we currently have the base ability.
			else
			{
				//If the selection is the base ability class, return.
				if (!IsValid(NewSelection.Selection))
				{
					return;
				}
				TalentChoice.CurrentSelection = NewSelection.Selection;
				//Learn the new talent.
				TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, NewSelection.Selection);
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.TalentRow.BaseAbilityClass);
				}
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(TalentChoice.TalentRow.BaseAbilityClass, nullptr, TalentChoice.CurrentSelection);
			}
			break;
		}
	}
}