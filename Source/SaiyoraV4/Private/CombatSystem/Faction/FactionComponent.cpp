#include "Faction/FactionComponent.h"

UFactionComponent::UFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFactionComponent::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->GetComponents(OwnerMeshes);
	UpdateOwnerCustomRendering();
}

void UFactionComponent::UpdateOwnerCustomRendering()
{
	int32 const StencilIndex = static_cast<int>(DefaultFaction) + 1;
	for (UMeshComponent* Mesh : OwnerMeshes)
	{
		if (IsValid(Mesh))
		{
			int32 const PreviousStencil = Mesh->CustomDepthStencilValue;
			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue((PreviousStencil - (PreviousStencil % 10)) + StencilIndex);
		}
	}
}
