#include "Faction/FactionComponent.h"
#include "SaiyoraCombatInterface.h"
#include "Kismet/GameplayStatics.h"

UFactionComponent::UFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFactionComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Faction Component."));
	GetOwner()->GetComponents(OwnerMeshes);
	const APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(LocalPC))
	{
		const APawn* LocalPawn = LocalPC->GetPawn();
		if (IsValid(LocalPawn) && LocalPawn == GetOwner())
		{
			bIsLocalPlayer = true;
		}
	}
	UpdateOwnerCustomRendering();
}

void UFactionComponent::UpdateOwnerCustomRendering()
{
	//Local player gets final stencil value of 200, other players get 100-199, NPCs get 0-99.
	const int32 StencilIndex = bIsLocalPlayer ? 2 : DefaultFaction == EFaction::Player ? 1 : 0;
	for (UMeshComponent* Mesh : OwnerMeshes)
	{
		if (IsValid(Mesh))
		{
			const int32 PreviousStencil = Mesh->CustomDepthStencilValue;
			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue((StencilIndex * 100) + (PreviousStencil % 100));
		}
	}
}
