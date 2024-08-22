#include "ResourceContainer.h"
#include "PlayerHUD.h"
#include "ResourceBar.h"
#include "ResourceHandler.h"
#include "SaiyoraPlayerCharacter.h"
#include "VerticalBox.h"
#include "VerticalBoxSlot.h"

void UResourceContainer::InitResourceContainer(ASaiyoraPlayerCharacter* OwningPlayer)
{
	if (!IsValid(OwningPlayer)
		|| !OwningPlayer->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	OwnerResource = ISaiyoraCombatInterface::Execute_GetResourceHandler(OwningPlayer);
	if (!IsValid(OwnerResource))
	{
		return;
	}
	OwnerResource->OnResourceAdded.AddDynamic(this, &UResourceContainer::OnResourceAdded);
	OwnerResource->OnResourceRemoved.AddDynamic(this, &UResourceContainer::OnResourceRemoved);
	TArray<UResource*> PlayerResources;
	OwnerResource->GetActiveResources(PlayerResources);
	for (UResource* Resource : PlayerResources)
	{
		OnResourceAdded(Resource);
	}
}

void UResourceContainer::OnResourceAdded(UResource* AddedResource)
{
	if (!IsValid(ResourceBox))
	{
		return;
	}
	if (!IsValid(AddedResource))
	{
		return;
	}
	const TSubclassOf<UResourceBar> WidgetClass = AddedResource->GetHUDResourceWidget();
	if (!IsValid(WidgetClass))
	{
		return;
	}
	UResourceBar* ResourceWidget = CreateWidget<UResourceBar>(this, WidgetClass);
	if (!IsValid(ResourceWidget))
	{
		return;
	}
	ResourceWidget->InitResourceBar(AddedResource);
	ResourceWidgets.Add(AddedResource, ResourceWidget);
	UVerticalBoxSlot* BoxSlot = ResourceBox->AddChildToVerticalBox(ResourceWidget);
	if (!IsValid(BoxSlot))
	{
		return;
	}
	//TODO: Modify vertical box slot.
}

void UResourceContainer::OnResourceRemoved(UResource* RemovedResource)
{
	if (!IsValid(ResourceBox) || !IsValid(RemovedResource))
	{
		return;
	}
	UResourceBar* ResourceWidget = nullptr;
	ResourceWidgets.RemoveAndCopyValue(RemovedResource, ResourceWidget);
	if (IsValid(ResourceWidget))
	{
		ResourceWidget->RemoveFromParent();
	}
}