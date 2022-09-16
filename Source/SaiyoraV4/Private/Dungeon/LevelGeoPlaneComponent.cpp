#include "LevelGeoPlaneComponent.h"
#include "PlaneComponent.h"
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
			LocalPlayerPlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(LocalPawn);
			if (IsValid(LocalPlayerPlaneComponent))
			{
				LocalPlayerPlaneComponent->OnPlaneSwapped.AddDynamic(this, &ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap);
				bIsRenderedXPlane = UPlaneComponent::CheckForXPlane(LocalPlayerPlaneComponent->GetCurrentPlane(), DefaultPlane);
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
	}
	
}

void ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane,
	UObject* Source)
{
	const bool bPreviouslyXPlane = bIsRenderedXPlane;
	bIsRenderedXPlane = UPlaneComponent::CheckForXPlane(NewPlane, DefaultPlane);
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
}
