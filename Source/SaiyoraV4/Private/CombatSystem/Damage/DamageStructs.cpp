#include "DamageStructs.h"

#include "Buff.h"
#include "DamageBuffFunctions.h"

FPendingResurrection::FPendingResurrection(const UPendingResurrectionFunction* InResFunction)
{
	BuffSource = InResFunction->GetOwningBuff();
	CallbackName = InResFunction->CallbackFunctionName;
	if (IsValid(BuffSource))
	{
		Icon = BuffSource->GetBuffIcon();
	}
	bOverrideHealth = InResFunction->bOverrideHealth;
	OverrideHealthPercent = InResFunction->HealthPercentageOverride;
	ResLocation = InResFunction->ResLocation;
	static int ResIDCounter = 0;
	ResID = ResIDCounter++;
}