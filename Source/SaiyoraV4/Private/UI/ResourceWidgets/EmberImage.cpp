#include "ResourceWidgets/EmberImage.h"
#include "DamageEnums.h"
#include "Image.h"
#include "SaiyoraUIDataAsset.h"
#include "UIFunctionLibrary.h"

void UEmberImage::SetActive(const bool bActive)
{
	//TODO: Play some animations and stuff to ignite or extinguish the ember.
	if (IsValid(EmberImage))
	{
		if (bActive)
		{
			const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
			EmberImage->SetBrushTintColor(IsValid(UIDataAsset) ? UIDataAsset->GetSchoolColor(EElementalSchool::Fire) : FLinearColor::White);
		}
		else
		{
			EmberImage->SetBrushTintColor(FLinearColor(.1f, .1f, .1f, .05f));
		}
	}
}