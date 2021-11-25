#include "Faction/FactionComponent.h"
#include "SaiyoraCombatInterface.h"

UFactionComponent::UFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFactionComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Faction Component."), *GetOwner()->GetActorLabel());
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
