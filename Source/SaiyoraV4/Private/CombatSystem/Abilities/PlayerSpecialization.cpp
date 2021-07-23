// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerSpecialization.h"
#include "UnrealNetwork.h"

void UPlayerSpecialization::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerSpecialization, OwningComponent);
}

void UPlayerSpecialization::InitializeSpecObject(UPlayerAbilityHandler* AbilityHandler)
{
	if (bInitialized || !IsValid(AbilityHandler))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bad initialization in PlayerSpec object."));
		return;
	}
	OwningComponent = AbilityHandler;
	for (TTuple<TSubclassOf<UCombatAbility>, FAbilityTalentInfo> const& SpecAbility : SpecAbilities)
	{
		//Use the baseline ability class, since no talents were passed.
		SelectedTalents.Add(SpecAbility.Key, SpecAbility.Key);
	}
	SetupSpecObject();
	bInitialized = true;
}

void UPlayerSpecialization::InitializeSpecObject(UPlayerAbilityHandler* AbilityHandler,
	TMap<TSubclassOf<UCombatAbility>, TSubclassOf<UCombatAbility>> const& TalentSetup)
{
	if (bInitialized || !IsValid(AbilityHandler))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bad initialization in PlayerSpec object."));
		return;
	}
	//If passed empty talents, just use the default version of this function.
	if (TalentSetup.Num() == 0)
	{
		InitializeSpecObject(AbilityHandler);
		return;
	}
	OwningComponent = AbilityHandler;
	for (TTuple<TSubclassOf<UCombatAbility>, FAbilityTalentInfo> const& SpecAbility : SpecAbilities)
	{
		TSubclassOf<UCombatAbility> const* SelectedTalent = TalentSetup.Find(SpecAbility.Key);
		if (SelectedTalent)
		{
			SelectedTalents.Add(SpecAbility.Key, *SelectedTalent);
		}
		else
		{
			SelectedTalents.Add(SpecAbility.Key, SpecAbility.Key);
		}
	}
	SetupSpecObject();
	bInitialized = true;
}

void UPlayerSpecialization::UnlearnSpecObject()
{
	DeactivateSpecObject();
	bInitialized = false;
}

void UPlayerSpecialization::CreateNewDefaultLoadout(FPlayerAbilityLoadout& OutLoadout)
{
	OutLoadout.EmptyLoadout();
	for (int i = 0; i < GrantedAbilities.Num(); i++)
	{
		switch (PrimaryBar)
		{
			case EActionBarType::Ancient :
				OutLoadout.AncientLoadout.Add(i, GrantedAbilities[i]);
				break;
			case EActionBarType::Modern :
				OutLoadout.ModernLoadout.Add(i, GrantedAbilities[i]);
				break;
			default :
				break;
		}
	}
}

void UPlayerSpecialization::OnRep_OwningComponent()
{
	if (bInitialized || !IsValid(OwningComponent))
	{
		return;
	}
	SetupSpecObject();
	bInitialized = true;
	OwningComponent->NotifyOfNewSpecObject(this);
}

bool UPlayerSpecialization::GetTalentInfo(TSubclassOf<UCombatAbility> const AbilityClass,
	FAbilityTalentInfo& OutInfo) const
{
	//TODO:Talent system and UI integration.
	return true;
}
