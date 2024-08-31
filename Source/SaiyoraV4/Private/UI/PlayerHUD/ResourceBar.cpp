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
		FResourceState(OwnerResource->GetMaximum(), OwnerResource->GetCurrentValue()),
		FResourceState(OwnerResource->GetMaximum(), OwnerResource->GetCurrentValue()));
}