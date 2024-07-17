#include "SaiyoraUIDataAsset.h"

#include "DamageEnums.h"

USaiyoraUIDataAsset::USaiyoraUIDataAsset()
{
	SchoolColors = TMap<EElementalSchool, FLinearColor>({
		//None: Grey
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::None, FLinearColor(0.6f, 0.6f, 0.6f)),
		//Physical: White
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Physical, FLinearColor(1.0f, 1.0f, 1.0f)),
		//Sky: Pink
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Sky, FLinearColor(1.0f, 0.0f, 1.0f)),
		//Fire: Orange
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Fire, FLinearColor(1.0f, 0.5f, 0.0f)),
		//Water: Light Blue
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Water, FLinearColor(0.2f, 1.0f, 1.0f)),
		//Earth: Green
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Earth, FLinearColor(0.0f, 0.8f, 0.0f)),
		//Military: Purple
		TTuple<EElementalSchool, FLinearColor>(EElementalSchool::Military, FLinearColor(0.6f, 0.2f, 1.0f))
		});
}