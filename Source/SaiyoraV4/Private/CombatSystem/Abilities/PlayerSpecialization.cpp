// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatSystem/Abilities/PlayerSpecialization.h"

#include "UnrealNetwork.h"

void UPlayerSpecialization::InitializeSpecObject(UPlayerAbilityHandler* AbilityHandler)
{
	if (bInitialized || !IsValid(AbilityHandler))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bad initialization in PlayerSpec object."));
		return;
	}
	OwningComponent = AbilityHandler;
	SetupSpecObject();
	bInitialized = true;
}

void UPlayerSpecialization::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UPlayerSpecialization, OwningComponent, COND_OwnerOnly);
}

void UPlayerSpecialization::OnRep_OwningComponent()
{
	if (bInitialized || !IsValid(OwningComponent))
	{
		return;
	}
	SetupSpecObject();
	bInitialized = true;
}

EActionBarType UPlayerSpecialization::GetGrantedAbilities(TSet<TSubclassOf<UCombatAbility>>& OutAbilities) const
{
	OutAbilities = GrantedAbilities;
	return PrimaryBar;
}

bool UPlayerSpecialization::GetTalentInfo(TSubclassOf<UCombatAbility> const AbilityClass,
	FAbilityTalentInfo& OutInfo) const
{
	if (!IsValid(AbilityClass))
	{
		return false;
	}
	FAbilityTalentInfo const* FoundInfo = TalentInformation.Find(AbilityClass);
	if (!FoundInfo)
	{
		return false;
	}
	OutInfo = *FoundInfo;
	return true;
}
