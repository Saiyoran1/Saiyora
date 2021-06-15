// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"

#include "PlayerAbilitySave.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlaneComponent.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

const int32 UPlayerAbilityHandler::AbilitiesPerBar = 6;

UPlayerAbilityHandler::UPlayerAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UPlayerAbilityHandler::InitializeComponent()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	ACharacter* OwningPlayer = Cast<ACharacter>(GetOwner());
	if (IsValid(OwningPlayer))
	{
		PlayerStateRef = OwningPlayer->GetPlayerState();
	}
	if (IsValid(PlayerStateRef))
	{
		FString SaveName = PlayerStateRef->GetPlayerName();
		SaveName.Append(TEXT("Abilities"));
		AbilitySave = Cast<UPlayerAbilitySave>(UGameplayStatics::LoadGameFromSlot(SaveName, 0));
	}
	if (IsValid(AbilitySave))
	{
		if (GetOwnerRole() == ROLE_Authority)
		{
			AbilitySave->GetUnlockedAbilities(Spellbook);
		}
		AbilitySave->GetLastSavedLoadout(Loadout);
	}
	else
	{
		AbilitySave = NewObject<UPlayerAbilitySave>(GetOwner(), UPlayerAbilitySave::StaticClass());
		AbilitySave->GetLastSavedLoadout(Loadout);
	}
	FAbilityRestriction PlaneAbilityRestriction;
	PlaneAbilityRestriction.BindUFunction(this, FName(TEXT("CheckForCorrectAbilityPlane")));
	AddAbilityRestriction(PlaneAbilityRestriction);
}

bool UPlayerAbilityHandler::CheckForCorrectAbilityPlane(UCombatAbility* Ability)
{
	if (!IsValid(Ability))
	{
		return false;
	}
	switch (Ability->GetAbilityPlane())
	{
		case ESaiyoraPlane::None :
			return false;
		case ESaiyoraPlane::Neither :
			return false;
		case ESaiyoraPlane::Both :
			return false;
		case ESaiyoraPlane::Ancient :
			return CurrentBar != EActionBarType::Ancient;
		case ESaiyoraPlane::Modern :
			return CurrentBar != EActionBarType::Modern;
		default :
			return false;
	}
}

void UPlayerAbilityHandler::BeginPlay()
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		USaiyoraPlaneComponent* PlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
		if (IsValid(PlaneComponent))
		{
			switch (PlaneComponent->GetCurrentPlane())
			{
				case ESaiyoraPlane::None :
					break;
				case ESaiyoraPlane::Neither :
					break;
				case ESaiyoraPlane::Both :
					break;
				case ESaiyoraPlane::Ancient :
					CurrentBar = EActionBarType::Ancient;
					break;
				case ESaiyoraPlane::Modern :
					CurrentBar = EActionBarType::Modern;
					break;
				default :
					break;
			}
			FPlaneSwapCallback PlaneSwapCallback;
			PlaneSwapCallback.BindUFunction(this, FName(TEXT("UpdateAbilityPlane")));
			PlaneComponent->SubscribeToPlaneSwap(PlaneSwapCallback);
		}
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.AncientLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.ModernLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.HiddenLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
	}
}

void UPlayerAbilityHandler::UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source)
{
	if (NewPlane == ESaiyoraPlane::Ancient || NewPlane == ESaiyoraPlane::Modern)
	{
		EActionBarType const PreviousBar = CurrentBar;
		CurrentBar = NewPlane == ESaiyoraPlane::Ancient ? EActionBarType::Ancient : EActionBarType::Modern;
		if (PreviousBar != CurrentBar)
		{
			OnBarSwap.Broadcast(CurrentBar);
		}
	}
}

void UPlayerAbilityHandler::AbilityInput(int32 const BindNumber, bool const bHidden)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		return;
	}
	TSubclassOf<UCombatAbility> AbilityClass;
	if (bHidden)
	{
		AbilityClass = Loadout.HiddenLoadout.FindRef(BindNumber);
	}
	else
	{
		switch (CurrentBar)
		{
			case EActionBarType::Ancient :
				AbilityClass = Loadout.AncientLoadout.FindRef(BindNumber);
				break;
			case EActionBarType::Modern :
				AbilityClass = Loadout.ModernLoadout.FindRef(BindNumber);
				break;
			default :
				break;
		}
	}
	if (IsValid(AbilityClass))
	{
		UseAbility(AbilityClass);
	}
}

void UPlayerAbilityHandler::LearnAbility(TSubclassOf<UCombatAbility> const NewAbility)
{
	if (!IsValid(NewAbility) || Spellbook.Contains(NewAbility))
	{
		return;
	}
	Spellbook.Add(NewAbility);
	//TODO: Spellbook delegate? Can't use TSubclassOf vars in the delegate I think.
}

void UPlayerAbilityHandler::SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityBindUpdated.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityBindUpdated.Remove(Callback);
}

void UPlayerAbilityHandler::SubscribeToBarSwap(FBarSwapCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnBarSwap.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromBarSwap(FBarSwapCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnBarSwap.Remove(Callback);
}