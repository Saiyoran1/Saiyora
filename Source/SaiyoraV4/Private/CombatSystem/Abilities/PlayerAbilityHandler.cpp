// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"

const int32 UPlayerAbilityHandler::AbilitiesPerBar = 7;

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
		AbilityAddCallback.BindUFunction(this, FName(TEXT("AddNewAbilityToBars")));
		SubscribeToAbilityAdded(AbilityAddCallback);
	}
}

void UPlayerAbilityHandler::AddNewAbilityToBars(UCombatAbility* NewAbility)
{
	if (!IsValid(NewAbility) || NewAbility->GetHiddenOnActionBar())
	{
		return;
	}
	int32 Bind = -1;
	switch (NewAbility->GetAbilityPlane())
	{
		case ESaiyoraPlane::None :
			break;
		case ESaiyoraPlane::Neither :
			break;
		case ESaiyoraPlane::Both :
			{
				Bind = FindFirstOpenAbilityBind(ESaiyoraPlane::Modern);
				if (Bind != -1)
				{
					ModernAbilities.Add(Bind, NewAbility);
					OnAbilityBindUpdated.Broadcast(Bind, ESaiyoraPlane::Modern, NewAbility);
				}
				Bind = FindFirstOpenAbilityBind(ESaiyoraPlane::Ancient);
				if (Bind != -1)
				{
					AncientAbilities.Add(Bind, NewAbility);
					OnAbilityBindUpdated.Broadcast(Bind, ESaiyoraPlane::Ancient, NewAbility);
				}
			}
			break;
		case ESaiyoraPlane::Ancient :
			{
				Bind = FindFirstOpenAbilityBind(ESaiyoraPlane::Ancient);
				if (Bind != -1)
				{
					AncientAbilities.Add(Bind, NewAbility);
					OnAbilityBindUpdated.Broadcast(Bind, ESaiyoraPlane::Ancient, NewAbility);
				}
			}
			break;
		case ESaiyoraPlane::Modern :
			{
				Bind = FindFirstOpenAbilityBind(ESaiyoraPlane::Modern);
				if (Bind != -1)
				{
					ModernAbilities.Add(Bind, NewAbility);
					OnAbilityBindUpdated.Broadcast(Bind, ESaiyoraPlane::Modern, NewAbility);
				}
			}
			break;
		default :
			break;
	}
}

int32 UPlayerAbilityHandler::FindFirstOpenAbilityBind(ESaiyoraPlane const BarPlane) const
{
	if (BarPlane == ESaiyoraPlane::Both || BarPlane == ESaiyoraPlane::Neither || BarPlane == ESaiyoraPlane::None)
	{
		return -1;
	}
	TSet<int32> UsedBinds;
	if (BarPlane == ESaiyoraPlane::Ancient)
	{
		AncientAbilities.GetKeys(UsedBinds);
	}
	else
	{
		ModernAbilities.GetKeys(UsedBinds);
	}
	int32 OpenBind = -1;
	for (int32 i = 0; i < AbilitiesPerBar; i++)
	{
		if (!UsedBinds.Contains(i))
		{
			OpenBind = i;
			break;
		}
	}
	return OpenBind;
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
