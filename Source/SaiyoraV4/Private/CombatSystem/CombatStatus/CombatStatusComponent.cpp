#include "CombatStatusComponent.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

TMap<int32, UCombatStatusComponent*> UCombatStatusComponent::StencilValues = TMap<int32, UCombatStatusComponent*>();

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
	if (StencilValues.Num() == 0)
	{
		//This is the first CombatStatusComponent to initialize, fill out the RenderingIDs map.
		for (int32 i = 1; i <= 49; i++)
		{
			StencilValues.Add(i, nullptr);
		}
		for (int32 i = 51; i <= 99; i++)
		{
			StencilValues.Add(i, nullptr);
		}
		for (int32 i = 101; i <= 149; i++)
		{
			StencilValues.Add(i, nullptr);
		}
		for (int32 i = 151; i <= 199; i++)
		{
			StencilValues.Add(i, nullptr);
		}
		for (int32 i = 201; i <= 255; i++)
		{
			StencilValues.Add(i, nullptr);
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
	if (UpdateStencilValue())
	{
		for (UMeshComponent* Mesh : OwnerMeshes)
		{
			if (IsValid(Mesh))
			{
				Mesh->SetRenderCustomDepth(bUseCustomDepth);
				if (bUseCustomDepth)
				{
					Mesh->SetCustomDepthStencilValue(StencilValue);
				}
			}
		}
	}
}

bool UCombatStatusComponent::UpdateStencilValue()
{
	/*
	 * 0 = Enemy, same Plane, Out of IDs
	 * 1-49 = Enemy, same Plane
	 * 50 = Enemy, XPlane, Out of IDs
	 * 51-99 = Enemy, XPlane
	 * 100 = Neutral, same Plane, Out of IDs
	 * 101-149 = Neutral, same Plane
	 * 150 = Neutral, XPlane, Out of IDs
	 * 151-199 = Neutral, XPlane
	 * 200 = Local player, or no outlines
	 * 201-227 = Friendly, same Plane
	 * 228-254 = Friendly, XPlane
	 */
	const int32 PreviousStencil = StencilValue;
	const bool bPreviouslyUsingCustomDepth = bUseCustomDepth;
	if (!IsValid(LocalPlayerStatusComponent) || LocalPlayerStatusComponent == this)
	{
		StencilValue = 200;
		bUseCustomDepth = false;
	}
	else
	{
		bUseCustomDepth = true;
		
		int32 RangeStart = 200;
		int32 RangeEnd = 200;
		int32 DefaultID = 200;
		const bool bIsXPlane = CheckForXPlane(LocalPlayerStatusComponent->GetCurrentPlane(), GetCurrentPlane());
		switch (GetCurrentFaction())
		{
		case EFaction::Friendly :
			RangeStart = bIsXPlane ? 228 : 201;
			RangeEnd = bIsXPlane ? 254 : 227;
			DefaultID = 200;
			break;
		case EFaction::Neutral :
			RangeStart = bIsXPlane ? 151 : 101;
			RangeEnd = bIsXPlane ? 199 : 149;
			DefaultID = bIsXPlane ? 150 : 100;
			break;
		case EFaction::Enemy :
			RangeStart = bIsXPlane ? 51 : 1;
			RangeEnd = bIsXPlane ? 99 : 49;
			DefaultID = bIsXPlane ? 50 : 0;
			break;
		default :
			break;
		}
		if (StencilValue < RangeStart || StencilValue > RangeEnd)
		{
			bool bFoundNewID = false;
			for (int32 i = RangeStart; i <= RangeEnd; i++)
			{
				if (StencilValues.Contains(i) && StencilValues.FindRef(i) == nullptr)
				{
					StencilValues.Add(i, this);
					StencilValue = i;
					bFoundNewID = true;
					break;
				}
			}
			if (!bFoundNewID)
			{
				StencilValue = DefaultID;
			}
		}
	}
	if (PreviousStencil != StencilValue && bPreviouslyUsingCustomDepth && StencilValues.Contains(PreviousStencil))
	{
		StencilValues.Add(PreviousStencil, nullptr);
	}
	return PreviousStencil != StencilValue || bPreviouslyUsingCustomDepth != bUseCustomDepth;
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
				Component->SetCollisionProfileName(FSaiyoraCollision::P_AncientPawn);
				break;
			case ESaiyoraPlane::Modern :
				Component->SetCollisionProfileName(FSaiyoraCollision::P_ModernPawn);
				break;
			default :
				Component->SetCollisionProfileName(FSaiyoraCollision::P_Pawn);
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