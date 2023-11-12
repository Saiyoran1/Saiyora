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
		OwnerAbilityCompRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningPlayer);
		if (IsValid(OwnerAbilityCompRef))
		{
			for (const TSubclassOf<UCombatAbility> CoreAbilityClass : CoreAbilities)
			{
				OwnerAbilityCompRef->AddNewAbility(CoreAbilityClass);
			}
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
		if (IsValid(OwnerAbilityCompRef))
		{
			for (const TSubclassOf<UCombatAbility> CoreAbilityClass : CoreAbilities)
			{
				OwnerAbilityCompRef->RemoveAbility(CoreAbilityClass);
			}
			for (const FModernTalentChoice& TalentChoice : Loadout.Items)
			{
				OwnerAbilityCompRef->RemoveAbility(TalentChoice.Selection);
			}
		}
	}
	OnUnlearn();
}

void UModernSpecialization::SelectModernAbility(const int32 Slot, const TSubclassOf<UCombatAbility> Ability)
{
	if (GetOwningPlayer()->GetLocalRole() != ROLE_Authority || Slot < 0 || !IsValid(Ability))
	{
		return;
	}
	if (!IsValid(AbilityPool) || !AbilityPool->ModernAbilityPool.Contains(Ability))
	{
		return;
	}
	if (IsValid(OwnerAbilityCompRef) && IsValid(OwnerAbilityCompRef->FindActiveAbility(Ability)))
	{
		return;
	}
	bool bFound = false;
	TSubclassOf<UCombatAbility> PreviousAbility = nullptr;
	for (FModernTalentChoice& TalentChoice : Loadout.Items)
	{
		if (TalentChoice.SlotNumber == Slot)
		{
			if (TalentChoice.Selection == Ability)
			{
				return;
			}
			bFound = true;
			PreviousAbility = TalentChoice.Selection;
			TalentChoice.Selection = Ability;
			Loadout.MarkItemDirty(TalentChoice);
			if (IsValid(OwnerAbilityCompRef))
			{
				OwnerAbilityCompRef->RemoveAbility(PreviousAbility);
				OwnerAbilityCompRef->AddNewAbility(Ability);
			}
			break;
		}
	}
	if (!bFound)
	{
		Loadout.MarkItemDirty(Loadout.Items.Add_GetRef(FModernTalentChoice(Slot, Ability)));
		if (IsValid(OwnerAbilityCompRef))
		{
			OwnerAbilityCompRef->AddNewAbility(Ability);
		}
	}
	OnTalentChanged.Broadcast(Slot, PreviousAbility, Ability);
}

void UModernSpecialization::ClearAbilitySelection(const int32 Slot)
{
	if (GetOwningPlayer()->GetLocalRole() != ROLE_Authority || Slot < 0)
	{
		return;
	}
	for (FModernTalentChoice& TalentChoice : Loadout.Items)
	{
		if (TalentChoice.SlotNumber == Slot)
		{
			if (IsValid(TalentChoice.Selection))
			{
				const TSubclassOf<UCombatAbility> PreviousAbility = TalentChoice.Selection;
				if (IsValid(OwnerAbilityCompRef))
				{
					OwnerAbilityCompRef->RemoveAbility(PreviousAbility);
				}
				TalentChoice.Selection = nullptr;
				Loadout.MarkItemDirty(TalentChoice);
				OnTalentChanged.Broadcast(Slot, PreviousAbility, nullptr);
			}
			return;
		}
	}
}
