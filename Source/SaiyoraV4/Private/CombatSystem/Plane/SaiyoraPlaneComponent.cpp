// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraPlaneComponent.h"
#include "UnrealNetwork.h"

USaiyoraPlaneComponent::USaiyoraPlaneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void USaiyoraPlaneComponent::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USaiyoraPlaneComponent, PlaneStatus);
}

void USaiyoraPlaneComponent::InitializeComponent()
{
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
}

ESaiyoraPlane USaiyoraPlaneComponent::PlaneSwap(bool const bIgnoreRestrictions, UObject* Source,
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

void USaiyoraPlaneComponent::AddPlaneSwapRestriction(FPlaneSwapCondition const& Condition)
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

void USaiyoraPlaneComponent::RemovePlaneSwapRestriction(FPlaneSwapCondition const& Condition)
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

void USaiyoraPlaneComponent::SubscribeToPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.AddUnique(Callback);
	}
}

void USaiyoraPlaneComponent::UnsubscribeFromPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.Remove(Callback);
	}
}

void USaiyoraPlaneComponent::OnRep_PlaneStatus(FPlaneStatus const PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
	}
}