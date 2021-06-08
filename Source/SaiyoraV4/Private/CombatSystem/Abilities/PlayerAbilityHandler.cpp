// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"

#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlaneComponent.h"

const int32 UPlayerAbilityHandler::AbilitiesPerBar = 6;

UPlayerAbilityHandler::UPlayerAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UPlayerAbilityHandler::InitializeComponent()
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FAbilityInstanceCallback AbilityAddCallback;
		AbilityAddCallback.BindUFunction(this, FName(TEXT("SetupNewAbilityBind")));
		SubscribeToAbilityAdded(AbilityAddCallback);
	}
}

void UPlayerAbilityHandler::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		USaiyoraPlaneComponent* PlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
		if (IsValid(PlaneComponent))
		{
			AbilityPlane = PlaneComponent->GetCurrentPlane();
			FPlaneSwapCallback PlaneSwapCallback;
			PlaneSwapCallback.BindUFunction(this, FName(TEXT("UpdateAbilityPlane")));
			PlaneComponent->SubscribeToPlaneSwap(PlaneSwapCallback);
		}
	}
}

void UPlayerAbilityHandler::SetupNewAbilityBind(UCombatAbility* NewAbility)
{
	if (!IsValid(NewAbility) || GetOwnerRole() != ROLE_AutonomousProxy)
	{
		return;
	}
	
}

void UPlayerAbilityHandler::UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source)
{
	if ((NewPlane == ESaiyoraPlane::Ancient || NewPlane == ESaiyoraPlane::Modern) && NewPlane != AbilityPlane)
	{
		AbilityPlane = NewPlane;
		OnBarSwap.Broadcast(NewPlane);
	}
}

void UPlayerAbilityHandler::AbilityInput(int32 const BindNumber, bool const bHidden)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		return;
	}
	UCombatAbility* Ability = nullptr;
	if (bHidden)
	{
		Ability = HiddenBinds.FindRef(BindNumber);
	}
	else
	{
		if (AbilityPlane == ESaiyoraPlane::Both || AbilityPlane == ESaiyoraPlane::Neither || AbilityPlane == ESaiyoraPlane::None)
		{
			return;
		}
		if (AbilityPlane == ESaiyoraPlane::Ancient)
		{
			Ability = AncientBinds.FindRef(BindNumber);
		}
		else
		{
			Ability = ModernBinds.FindRef(BindNumber);
		}
	}
	if (IsValid(Ability))
	{
		UseAbility(Ability->GetClass());
	}
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
