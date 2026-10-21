#include "ResourceWidgets/EmberImage.h"
#include "DamageEnums.h"
#include "Image.h"
#include "SaiyoraDevSettings.h"
#include "SaiyoraUIDataAsset.h"
#include "UIFunctionLibrary.h"

void UEmberImage::OnSetActive(const bool bNewActive)
{
	//TODO: Play some animations and stuff to ignite or extinguish the ember.
	//Might have to just write the anim logic in C++ and have a float curve settable in editor so we can actually use the data asset colors.
	if (IsValid(EmberImage))
	{
		if (bNewActive)
		{
			EmberImage->SetBrushTintColor(UI_DA->GetSchoolColor(EElementalSchool::Fire));
		}
		else
		{
			EmberImage->SetBrushTintColor(UI_DA->InactiveDiscreteResourceIconColor);
		}
	}
}