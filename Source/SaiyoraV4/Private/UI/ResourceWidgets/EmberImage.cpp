#include "ResourceWidgets/EmberImage.h"
#include "DamageEnums.h"
#include "Image.h"
#include "SaiyoraUIDataAsset.h"
#include "UIFunctionLibrary.h"

void UEmberImage::OnSetActive(const bool bActive)
{
	//TODO: Play some animations and stuff to ignite or extinguish the ember.
	if (IsValid(EmberImage))
	{
		const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
		if (bActive)
		{
			EmberImage->SetBrushTintColor(IsValid(UIDataAsset) ? UIDataAsset->GetSchoolColor(EElementalSchool::Fire) : FLinearColor::White);
		}
		else
		{
			EmberImage->SetBrushTintColor(IsValid(UIDataAsset) ? UIDataAsset->InactiveDiscreteResourceIconColor : FLinearColor(.1f, .1f, .1f, .1f));
		}
	}
}