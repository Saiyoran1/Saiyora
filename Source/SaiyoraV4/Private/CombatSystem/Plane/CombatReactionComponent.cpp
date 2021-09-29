// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatReactionComponent.h"
#include "UnrealNetwork.h"

UCombatReactionComponent::UCombatReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UCombatReactionComponent::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatReactionComponent, PlaneStatus);
}

void UCombatReactionComponent::InitializeComponent()
{
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
}

void UCombatReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	//TODO: Set owner's mesh components to render custom depth, set custom stencil value to (10 * XPlane from local player + 1 * Faction).
	
}

ESaiyoraPlane UCombatReactionComponent::PlaneSwap(bool const bIgnoreRestrictions, UObject* Source,
                                                  bool const bToSpecificPlane, ESaiyoraPlane const TargetPlane)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return PlaneStatus.CurrentPlane;
	}
	if (!bCanEverPlaneSwap)
	{
		return PlaneStatus.CurrentPlane;
	}
	if (!bIgnoreRestrictions)
	{
		for (FPlaneSwapCondition const& Condition : PlaneSwapRestrictions)
		{
			if (Condition.IsBound())
			{
				if (Condition.Execute(this, Source, bToSpecificPlane, TargetPlane))
				{
					return PlaneStatus.CurrentPlane;
				}
			}
		}
	}
	
	ESaiyoraPlane const PreviousPlane = PlaneStatus.CurrentPlane;
	
	if (bToSpecificPlane && TargetPlane != ESaiyoraPlane::None)
	{
		PlaneStatus.CurrentPlane = TargetPlane;
	}
	else
	{
		switch (PlaneStatus.CurrentPlane)
		{
			case ESaiyoraPlane::Ancient :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Modern;
				break;
			case ESaiyoraPlane::Modern :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Ancient;
				break;
			case ESaiyoraPlane::Both :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Neither;
				break;
			case ESaiyoraPlane::Neither :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Both;
				break;
			default :
				return PlaneStatus.CurrentPlane;
		}
	}
	
	PlaneStatus.LastSwapSource = Source;

	if (PreviousPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousPlane, PlaneStatus.CurrentPlane, Source);
	}

	return PlaneStatus.CurrentPlane;
}

void UCombatReactionComponent::AddPlaneSwapRestriction(FPlaneSwapCondition const& Condition)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (bCanEverPlaneSwap && Condition.IsBound())
	{
		PlaneSwapRestrictions.AddUnique(Condition);
	}
}

void UCombatReactionComponent::RemovePlaneSwapRestriction(FPlaneSwapCondition const& Condition)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (bCanEverPlaneSwap && Condition.IsBound())
	{
		PlaneSwapRestrictions.RemoveSingleSwap(Condition);
	}
}

void UCombatReactionComponent::SubscribeToPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.AddUnique(Callback);
	}
}

void UCombatReactionComponent::UnsubscribeFromPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.Remove(Callback);
	}
}

void UCombatReactionComponent::OnRep_PlaneStatus(FPlaneStatus const PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
	}
}


