#include "NPCAbility.h"

void UNPCAbility::GetAbilityLocationRules(FNPCAbilityLocationRules& OutLocationRules)
{
	if (LocationRules.LocationRuleType == ELocationRuleReferenceType::None)
	{
		return;
	}
	OutLocationRules = LocationRules;
	GetAbilityLocationRuleOrigin(OutLocationRules.RangeTargetActor, OutLocationRules.RangeTargetLocation);
}
