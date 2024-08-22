#include "PlayerHUD/ResourceBar.h"
#include "Resource.h"

void UResourceBar::InitResourceBar(UResource* Resource)
{
	if (!IsValid(Resource))
	{
		return;
	}
	OwnerResource = Resource;
	OnInit();
	Resource->OnResourceChanged.AddDynamic(this, &UResourceBar::OnResourceChanged);
	OnResourceChanged(OwnerResource, nullptr,
		FResourceState(OwnerResource->GetMinimum(), OwnerResource->GetMaximum(), OwnerResource->GetCurrentValue()),
		FResourceState(OwnerResource->GetMinimum(), OwnerResource->GetMaximum(), OwnerResource->GetCurrentValue()));
}