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
			for (const FAncientTalentChoice& TalentChoice : Loadout.Items)
			{
				OwnerAbilityComp->AddNewAbility(TalentChoice.BaseAbility);
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
						TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.BaseAbility);
					}
				}
				else if (TalentChoice.ActiveTalent->GetClass() != TalentChoice.CurrentSelection)
				{
					TalentChoice.ActiveTalent->UnselectTalent();
					TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, TalentChoice.CurrentSelection);
					if (IsValid(TalentChoice.ActiveTalent))
					{
						TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.BaseAbility);
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
				if (IsValid(TalentChoice.BaseAbility))
				{
					OwnerAbilityComponent->RemoveAbility(TalentChoice.BaseAbility);
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
		if (TalentChoice.BaseAbility == NewSelection.BaseAbility)
		{
			if (IsValid(TalentChoice.CurrentSelection))
			{
				//Currently have an active talent.
				if (TalentChoice.CurrentSelection == NewSelection.Selection)
				{
					return;
				}
				const TSubclassOf<UAncientTalent> PreviousTalent = TalentChoice.CurrentSelection;
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->UnselectTalent();
				}
				TalentChoice.CurrentSelection = NewSelection.Selection;
				TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, NewSelection.Selection);
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.BaseAbility);
				}
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(TalentChoice.BaseAbility, PreviousTalent, TalentChoice.CurrentSelection);
			}
			else
			{
				//Currently have the base ability.
				if (!IsValid(NewSelection.Selection))
				{
					return;
				}
				TalentChoice.CurrentSelection = NewSelection.Selection;
				TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, NewSelection.Selection);
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->SelectTalent(this, TalentChoice.BaseAbility);
				}
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(TalentChoice.BaseAbility, nullptr, TalentChoice.CurrentSelection);
			}
		}
	}
}