#include "CombatStatusComponent.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

TMap<int32, UCombatStatusComponent*> UCombatStatusComponent::RenderingIDs = TMap<int32, UCombatStatusComponent*>();

#pragma region Setup

UCombatStatusComponent::UCombatStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UCombatStatusComponent::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatStatusComponent, PlaneStatus);
}

void UCombatStatusComponent::InitializeComponent()
{
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
	if (RenderingIDs.Num() == 0)
	{
		//This is the first CombatStatusComponent to initialize, fill out the RenderingIDs map.
		for (int32 i = 1; i <= 49; i++)
		{
			RenderingIDs.Add(i, nullptr);
		}
		for (int32 i = 51; i <= 99; i++)
		{
			RenderingIDs.Add(i, nullptr);
		}
	}
}

void UCombatStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Plane Component."));
	const APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(LocalPC))
	{
		const APawn* LocalPawn = LocalPC->GetPawn();
		if (IsValid(LocalPawn))
		{
			LocalPlayerStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPawn);
			if (IsValid(LocalPlayerStatusComponent))
			{
				LocalPlayerStatusComponent->OnPlaneSwapped.AddDynamic(this, &UCombatStatusComponent::OnLocalPlayerPlaneSwap);
			}
		}
	}
	GetOwner()->GetComponents(OwnerMeshes);
	UpdateOwnerCustomRendering();
	UpdateOwnerPlaneCollision();
}

#pragma endregion
#pragma region Plane

ESaiyoraPlane UCombatStatusComponent::PlaneSwap(const bool bIgnoreRestrictions, UObject* Source,
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
		UpdateOwnerCustomRendering();
		UpdateOwnerPlaneCollision();
	}
	return PlaneStatus.CurrentPlane;
}

bool UCombatStatusComponent::CheckForXPlane(const ESaiyoraPlane FromPlane, const ESaiyoraPlane ToPlane)
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

void UCombatStatusComponent::OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
		UpdateOwnerCustomRendering();
		UpdateOwnerPlaneCollision();
	}
}

void UCombatStatusComponent::UpdateOwnerCustomRendering()
{
	/*
	 * 0 = XFaction, same Plane, Out of IDs
	 * 1-49 = XFaction, same Plane
	 * 50 = XFaction, XPlane, Out of IDs
	 * 51-99 = XFaction, XPlane
	 * 100 = Same Faction, same Plane, Out of IDs
	 * 101-149 = Same Faction, same Plane
	 * 150 = Same Faction, XPlane, Out of IDs
	 * 151-199 = Same Faction, XPlane
	 * 200 = Local player, or no outline
	 */
	const int32 PreviousStencil = StencilValue;
	if (!IsValid(LocalPlayerStatusComponent) || LocalPlayerStatusComponent == this)
	{
		StencilValue = 200;
		CurrentPlaneID = 0;
	}
	else
	{
		const int32 FactionID = LocalPlayerStatusComponent->GetCurrentFaction() != GetCurrentFaction() ? 0 : 100;
		if (CheckForXPlane(LocalPlayerStatusComponent->GetCurrentPlane(), GetCurrentPlane()))
		{
			AssignXPlaneID();
		}
		else
		{
			AssignSamePlaneID();
		}
		StencilValue = FactionID + CurrentPlaneID;
	}
	if (StencilValue != PreviousStencil)
	{
		for (UMeshComponent* Mesh : OwnerMeshes)
		{
			if (IsValid(Mesh))
			{
				Mesh->SetRenderCustomDepth(true);
				Mesh->SetCustomDepthStencilValue(StencilValue);
			}
		}
	}
}

void UCombatStatusComponent::AssignXPlaneID()
{
	if (CurrentPlaneID > 0 && CurrentPlaneID < 50)
	{
		//Already in the right range.
		return;
	}
	const int32 PreviousID = CurrentPlaneID;
	for (int32 i = 1; i < 50; i++)
	{
		if (RenderingIDs.FindRef(i) == nullptr)
		{
			RenderingIDs.Add(i, this);
			CurrentPlaneID = i;
			break;
		}
	}
	if (PreviousID == CurrentPlaneID)
	{
		CurrentPlaneID = 0;
	}
	if (PreviousID != CurrentPlaneID && PreviousID != 0 && PreviousID != 50)
	{
		RenderingIDs.Remove(PreviousID);
	}
}

void UCombatStatusComponent::AssignSamePlaneID()
{
	if (CurrentPlaneID > 50 && CurrentPlaneID < 100)
	{
		//Already in right range.
		return;
	}
	const int32 PreviousID = CurrentPlaneID;
	for (int32 i = 51; i < 100; i++)
	{
		if (RenderingIDs.FindRef(i) == nullptr)
		{
			RenderingIDs.Add(i, this);
			CurrentPlaneID = i;
			break;
		}
	}
	if (PreviousID == CurrentPlaneID)
	{
		CurrentPlaneID = 50;
	}
	if (PreviousID != CurrentPlaneID && PreviousID != 0 && PreviousID != 50)
	{
		RenderingIDs.Remove(PreviousID);
	}
}

void UCombatStatusComponent::UpdateOwnerPlaneCollision()
{
	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Component : Primitives)
	{
		if (Component->GetCollisionObjectType() == ECC_Pawn)
		{
			switch (GetCurrentPlane())
			{
			case ESaiyoraPlane::Ancient :
				Component->SetCollisionProfileName(FName("AncientPawn"));
				break;
			case ESaiyoraPlane::Modern :
				Component->SetCollisionProfileName(FName("ModernPawn"));
				break;
			default :
				Component->SetCollisionProfileName(FName("Pawn"));
			}
		}
	}
}

#pragma endregion
#pragma region Restrictions

void UCombatStatusComponent::AddPlaneSwapRestriction(UBuff* Source, const FPlaneSwapRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source) && Restriction.IsBound())
	{
		PlaneSwapRestrictions.Add(Source, Restriction);
	}
}

void UCombatStatusComponent::RemovePlaneSwapRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && bCanEverPlaneSwap && IsValid(Source))
	{
		PlaneSwapRestrictions.Remove(Source);
	}
}

#pragma endregion