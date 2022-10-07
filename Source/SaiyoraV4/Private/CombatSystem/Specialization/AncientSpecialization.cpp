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
						TalentChoice.ActiveTalent->SelectTalent(this);
					}
				}
				else if (TalentChoice.ActiveTalent->GetClass() != TalentChoice.CurrentSelection)
				{
					TalentChoice.ActiveTalent->UnselectTalent();
					TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, TalentChoice.CurrentSelection);
					if (IsValid(TalentChoice.ActiveTalent))
					{
						TalentChoice.ActiveTalent->SelectTalent(this);
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

void UAncientSpecialization::SelectAncientTalent(const TSubclassOf<UCombatAbility> BaseAbility,
	const TSubclassOf<UAncientTalent> TalentSelection)
{
	if (!GetOwningPlayer()->HasAuthority() || !IsValid(BaseAbility) || !IsValid(TalentSelection))
	{
		return;
	}
	for (FAncientTalentChoice& TalentChoice : Loadout.Items)
	{
		if (TalentChoice.BaseAbility == BaseAbility)
		{
			if (TalentChoice.Talents.Contains(TalentSelection) && TalentChoice.CurrentSelection != TalentSelection)
			{
				const TSubclassOf<UAncientTalent> PreviousTalent = TalentChoice.CurrentSelection;
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->UnselectTalent();
				}
				TalentChoice.CurrentSelection = TalentSelection;
				
				TalentChoice.ActiveTalent = NewObject<UAncientTalent>(OwningPlayer, TalentSelection);
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->SelectTalent(this);
				}
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(TalentChoice.BaseAbility, PreviousTalent, TalentChoice.CurrentSelection);
			}
			break;
		}
	}
}

void UAncientSpecialization::ClearTalentSelection(const TSubclassOf<UCombatAbility> BaseAbility)
{
	if (!OwningPlayer->HasAuthority() || !IsValid(BaseAbility))
	{
		return;
	}
	for (FAncientTalentChoice& TalentChoice : Loadout.Items)
	{
		if (TalentChoice.BaseAbility == BaseAbility)
		{
			if (IsValid(TalentChoice.CurrentSelection))
			{
				const TSubclassOf<UAncientTalent> PreviousTalent = TalentChoice.CurrentSelection;
				if (IsValid(TalentChoice.ActiveTalent))
				{
					TalentChoice.ActiveTalent->UnselectTalent();
				}
				TalentChoice.ActiveTalent = nullptr;
				TalentChoice.CurrentSelection = nullptr;
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(BaseAbility, PreviousTalent, nullptr);
			}
			break;
		}
	}
}