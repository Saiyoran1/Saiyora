#include "LevelGeoPlaneComponent.h"
#include "CombatStatusComponent.h"
#include "CombatStructs.h"
#include "SaiyoraCombatInterface.h"
#include "Kismet/GameplayStatics.h"

ULevelGeoPlaneComponent::ULevelGeoPlaneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelGeoPlaneComponent::BeginPlay()
{
	Super::BeginPlay();
	const APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(LocalPC))
	{
		const APawn* LocalPawn = LocalPC->GetPawn();
		if (IsValid(LocalPawn))
		{
			LocalPlayerCombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPawn);
			if (IsValid(LocalPlayerCombatStatusComponent))
			{
				LocalPlayerCombatStatusComponent->OnPlaneSwapped.AddDynamic(this, &ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap);
				bIsRenderedXPlane = UCombatStatusComponent::CheckForXPlane(LocalPlayerCombatStatusComponent->GetCurrentPlane(), DefaultPlane);
				TArray<UMeshComponent*> OwnerMeshes;
				GetOwner()->GetComponents<UMeshComponent>(OwnerMeshes);
				for (UMeshComponent* Mesh : OwnerMeshes)
				{
					FMeshMaterials MeshMaterials;
					TArray<FName> Names = Mesh->GetMaterialSlotNames();
					for (const FName Name : Names)
					{
						int32 Index = Mesh->GetMaterialIndex(Name);
						MeshMaterials.Materials.Add(Index, Mesh->GetMaterial(Index));
						if (bIsRenderedXPlane)
						{
							Mesh->SetMaterial(Index, XPlaneMaterial);
						}
					}
					Materials.Add(Mesh, MeshMaterials);
				}
			}
		}
		SetInitialCollision();
	}
	
}

void ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane,
	UObject* Source)
{
	const bool bPreviouslyXPlane = bIsRenderedXPlane;
	bIsRenderedXPlane = UCombatStatusComponent::CheckForXPlane(NewPlane, DefaultPlane);
	if (bIsRenderedXPlane == bPreviouslyXPlane)
	{
		return;
	}
	TArray<UMeshComponent*> Meshes;
	Materials.GetKeys(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		FMeshMaterials* MeshMaterials = Materials.Find(Mesh);
		if (MeshMaterials)
		{
			for (const TTuple<int32, UMaterialInterface*>& Material : MeshMaterials->Materials)
			{
				Mesh->SetMaterial(Material.Key, bIsRenderedXPlane ? XPlaneMaterial : Material.Value);
			}
		}
	}
	UpdateCameraCollision();
}

void ULevelGeoPlaneComponent::SetInitialCollision()
{
	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Component : Primitives)
	{
		if (Component->GetCollisionObjectType() == ECC_WorldDynamic)
		{
			switch (DefaultPlane)
			{
			case ESaiyoraPlane::Ancient :
				Component->SetCollisionObjectType(FSaiyoraCollision::O_WorldAncient);
				break;
			case ESaiyoraPlane::Modern :
				Component->SetCollisionObjectType(FSaiyoraCollision::O_WorldModern);
			case ESaiyoraPlane::Both :
				break;
			default:
				Component->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
				break;
			}
		}
	}
	UpdateCameraCollision();
}

void ULevelGeoPlaneComponent::UpdateCameraCollision()
{
	if (IsValid(LocalPlayerCombatStatusComponent))
	{
		const bool bXPlane = UCombatStatusComponent::CheckForXPlane(LocalPlayerCombatStatusComponent->GetCurrentPlane(), DefaultPlane);
		TArray<UPrimitiveComponent*> Primitives;
		GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Component : Primitives)
		{
			if (bXPlane)
			{
				Component->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			}
			else
			{
				Component->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
			}
		}
	}
}
