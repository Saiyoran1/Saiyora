#include "PlaneComponent.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

TMap<int32, UPlaneComponent*> UPlaneComponent::RenderingIDs = TMap<int32, UPlaneComponent*>();

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
	if (RenderingIDs.Num() == 0)
	{
		//This is the first PlaneComponent to initialize, fill out the RenderingIDs map.
		for (int32 i = 1; i <= 99; i++)
		{
			if (i != 50)
			{
				RenderingIDs.Add(i, nullptr);
			}
		}
	}
}

void UPlaneComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Plane Component."));
	const APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(LocalPC))
	{
		const APawn* LocalPawn = LocalPC->GetPawn();
		if (IsValid(LocalPawn))
		{
			LocalPlayerPlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(LocalPawn);
			if (IsValid(LocalPlayerPlaneComponent))
			{
				LocalPlayerPlaneComponent->OnPlaneSwapped.AddDynamic(this, &UPlaneComponent::OnLocalPlayerPlaneSwap);
			}
		}
	}
	GetOwner()->GetComponents(OwnerMeshes);
	UpdateRenderingID();
}

#pragma endregion
#pragma region Plane

ESaiyoraPlane UPlaneComponent::PlaneSwap(const bool bIgnoreRestrictions, UObject* Source,
                                                  const bool bToSpecificPlane, const ESaiyoraPlane TargetPlane)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverPlaneSwap)
	{
		return PlaneStatus.CurrentPlane;
	}
	if (!bIgnoreRestrictions)
	{
		for (const TTuple<UBuff*, FPlaneSwapRestriction>& Restriction : PlaneSwapRestrictions)
		{
			if (Restriction.Value.IsBound() && Restriction.Value.Execute(this, Source, bToSpecificPlane, TargetPlane))
			{
				return PlaneStatus.CurrentPlane;
			}
		}
	}
	const ESaiyoraPlane PreviousPlane = PlaneStatus.CurrentPlane;
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
		UpdateRenderingID();
	}
	return PlaneStatus.CurrentPlane;
}

bool UPlaneComponent::CheckForXPlane(const ESaiyoraPlane FromPlane, const ESaiyoraPlane ToPlane)
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

void UPlaneComponent::OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
		UpdateRenderingID();
	}
}

void UPlaneComponent::UpdateOwnerCustomRendering()
{
	if (IsValid(LocalPlayerPlaneComponent) && LocalPlayerPlaneComponent != this)
	{
		for (UMeshComponent* Mesh : OwnerMeshes)
		{
			if (IsValid(Mesh))
			{
				const int32 PreviousStencil = Mesh->CustomDepthStencilValue;
				Mesh->SetRenderCustomDepth(true);
				Mesh->SetCustomDepthStencilValue(((PreviousStencil / 100) * 100) + CurrentID);
			}
		}
	}
}

void UPlaneComponent::UpdateRenderingID()
{
	/* ID 0 = Actor is same plane, but no IDs remain OR Actor does not interact with plane rendering.
	** ID 1-49 = Actor is same plane.
	** ID 50 = Actor is xplane, but no IDs remain.
	** ID 51-99 = Actor is xplane.
	*/
	const int32 PreviousID = CurrentID;
	if (IsValid(LocalPlayerPlaneComponent) && LocalPlayerPlaneComponent != this)
	{
		const bool bShouldBeXPlane = CheckForXPlane(PlaneStatus.CurrentPlane, LocalPlayerPlaneComponent->GetCurrentPlane());
		const int32 StartingID = bShouldBeXPlane ? 51 : 1;
		const int32 EndingID = bShouldBeXPlane ? 99 : 49;
		if (StartingID <= PreviousID && PreviousID <= EndingID)
		{
			//We are already in the correct range of IDs.
			return;
		}
		for (int32 i = StartingID; i <= EndingID; i++)
		{
			if (RenderingIDs.FindRef(i) == nullptr)
			{
				RenderingIDs.Add(i, this);
				if (PreviousID != 0 && PreviousID != 50)
				{
					RenderingIDs.Add(PreviousID, nullptr);
					//TODO: Some kind of notification for components currently using ID 0/50 that an ID is available?
				}
				CurrentID = i;
				break;
			}
		}
		if (CurrentID == PreviousID)
		{
			CurrentID = bShouldBeXPlane ? 50 : 0;
		}
	}
	else
	{
		CurrentID = 0;
	}
	if (CurrentID != PreviousID)
	{
		UpdateOwnerCustomRendering();
	}
}

#pragma endregion
#pragma region Restrictions

void UPlaneComponent::AddPlaneSwapRestriction(UBuff* Source, const FPlaneSwapRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source) && Restriction.IsBound())
	{
		PlaneSwapRestrictions.Add(Source, Restriction);
	}
}

void UPlaneComponent::RemovePlaneSwapRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source))
	{
		PlaneSwapRestrictions.Remove(Source);
	}
}

#pragma endregion