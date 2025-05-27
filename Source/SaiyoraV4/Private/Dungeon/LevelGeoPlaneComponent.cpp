#include "LevelGeoPlaneComponent.h"
#include "AbilityFunctionLibrary.h"
#include "CombatStatusComponent.h"
#include "CombatStructs.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"

ULevelGeoPlaneComponent::ULevelGeoPlaneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelGeoPlaneComponent::BeginPlay()
{
	Super::BeginPlay();
	SetInitialCollision();
	const ASaiyoraPlayerCharacter* LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(this);
	if (IsValid(LocalPlayer))
	{
		LocalPlayerCombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPlayer);
		if (IsValid(LocalPlayerCombatStatusComponent))
		{
			SetupMaterialSwapping();
			UpdateCameraCollision();
		}
	}
	else
	{
		GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
		if (!IsValid(GameStateRef))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid game state ref in BeginPlay for LevelGeo Component."));
		}
		else
		{
			GameStateRef->OnPlayerAdded.AddDynamic(this, &ULevelGeoPlaneComponent::OnPlayerAdded);
		}
	}
}

void ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane,
	UObject* Source)
{
	const bool bPreviouslyXPlane = bIsRenderedXPlane;
	bIsRenderedXPlane = UAbilityFunctionLibrary::IsXPlane(NewPlane, DefaultPlane);
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
}

void ULevelGeoPlaneComponent::SetupMaterialSwapping()
{
	LocalPlayerCombatStatusComponent->OnPlaneSwapped.AddDynamic(this, &ULevelGeoPlaneComponent::OnLocalPlayerPlaneSwap);
	bIsRenderedXPlane = UAbilityFunctionLibrary::IsXPlane(LocalPlayerCombatStatusComponent->GetCurrentPlane(), DefaultPlane);
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

void ULevelGeoPlaneComponent::UpdateCameraCollision()
{
	if (IsValid(LocalPlayerCombatStatusComponent))
	{
		const bool bXPlane = UAbilityFunctionLibrary::IsXPlane(LocalPlayerCombatStatusComponent->GetCurrentPlane(), DefaultPlane);
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

void ULevelGeoPlaneComponent::OnPlayerAdded(ASaiyoraPlayerCharacter* NewPlayer)
{
	if (IsValid(NewPlayer) && NewPlayer->IsLocallyControlled())
	{
		LocalPlayerCombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(NewPlayer);
		if (IsValid(LocalPlayerCombatStatusComponent))
		{
			SetupMaterialSwapping();
			UpdateCameraCollision();
		}
		GameStateRef->OnPlayerAdded.RemoveDynamic(this, &ULevelGeoPlaneComponent::OnPlayerAdded);
	}
}
