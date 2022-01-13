#include "PlaneComponent.h"

#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

#pragma region Setup

UPlaneComponent::UPlaneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UPlaneComponent::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlaneComponent, PlaneStatus);
}

void UPlaneComponent::InitializeComponent()
{
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
	LocalPlayerSwapCallback.BindDynamic(this, &UPlaneComponent::OnLocalPlayerPlaneSwap);
}

void UPlaneComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Plane Component."), *GetOwner()->GetActorLabel());
	APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(LocalPC))
	{
		APawn* LocalPawn = LocalPC->GetPawn();
		if (IsValid(LocalPawn))
		{
			LocalPlayerPlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(LocalPawn);
			if (IsValid(LocalPlayerPlaneComponent))
			{
				LocalPlayerPlaneComponent->SubscribeToPlaneSwap(LocalPlayerSwapCallback);
			}
		}
	}
	GetOwner()->GetComponents(OwnerMeshes);
	UpdateOwnerCustomRendering();
}

#pragma endregion
#pragma region Plane

ESaiyoraPlane UPlaneComponent::PlaneSwap(bool const bIgnoreRestrictions, UObject* Source,
                                                  bool const bToSpecificPlane, ESaiyoraPlane const TargetPlane)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverPlaneSwap)
	{
		return PlaneStatus.CurrentPlane;
	}
	if (!bIgnoreRestrictions)
	{
		for (TTuple<UBuff*, FPlaneSwapRestriction> const& Restriction : PlaneSwapRestrictions)
		{
			if (Restriction.Value.IsBound() && Restriction.Value.Execute(this, Source, bToSpecificPlane, TargetPlane))
			{
				return PlaneStatus.CurrentPlane;
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
		UpdateOwnerCustomRendering();
	}
	return PlaneStatus.CurrentPlane;
}

bool UPlaneComponent::CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane)
{
	//None is the default value, always return false if it is provided.
	if (FromPlane == ESaiyoraPlane::None || ToPlane == ESaiyoraPlane::None)
	{
		return false;
	}
	//Actors "in between" planes will see everything as another plane.
	if (FromPlane == ESaiyoraPlane::Neither || ToPlane == ESaiyoraPlane::Neither)
	{
		return true;
	}
	//Actors in both planes will see everything except "in between" actors as the same plane.
	if (FromPlane == ESaiyoraPlane::Both || ToPlane == ESaiyoraPlane::Both)
	{
		return false;
	}
	//Actors in a normal plane will only see actors in the same plane or both planes as the same plane.
	return FromPlane != ToPlane;
}

void UPlaneComponent::OnRep_PlaneStatus(FPlaneStatus const PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
		UpdateOwnerCustomRendering();
	}
}

void UPlaneComponent::UpdateOwnerCustomRendering()
{
	if (IsValid(LocalPlayerPlaneComponent) && LocalPlayerPlaneComponent != this)
	{
		int32 StencilIndex = 0;
		StencilIndex += CheckForXPlane(PlaneStatus.CurrentPlane, LocalPlayerPlaneComponent->GetCurrentPlane()) ? 10 : 0;
		for (UMeshComponent* Mesh : OwnerMeshes)
		{
			if (IsValid(Mesh))
			{
				int32 const PreviousStencil = Mesh->CustomDepthStencilValue;
				Mesh->SetRenderCustomDepth(true);
				Mesh->SetCustomDepthStencilValue((PreviousStencil % 10) + StencilIndex);
			}
		}
	}
}

#pragma endregion
#pragma region Restrictions

void UPlaneComponent::AddPlaneSwapRestriction(UBuff* Source, FPlaneSwapRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source) && Restriction.IsBound())
	{
		PlaneSwapRestrictions.Add(Source, Restriction);
	}
}

void UPlaneComponent::RemovePlaneSwapRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source))
	{
		PlaneSwapRestrictions.Remove(Source);
	}
}

#pragma endregion
#pragma region Subscriptions

void UPlaneComponent::SubscribeToPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.AddUnique(Callback);
	}
}

void UPlaneComponent::UnsubscribeFromPlaneSwap(FPlaneSwapCallback const& Callback)
{
	if (bCanEverPlaneSwap && Callback.IsBound())
	{
		OnPlaneSwapped.Remove(Callback);
	}
}

#pragma endregion 