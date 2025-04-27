#include "DamageStructs.h"
#include "DamageBuffFunctions.h"

FPendingResurrection::FPendingResurrection(const UPendingResurrectionFunction* InResFunction)
{
	BuffSource = InResFunction->GetOwningBuff();
	CallbackName = InResFunction->CallbackFunctionName;
	bOverrideHealth = InResFunction->bOverrideHealth;
	OverrideHealthPercent = InResFunction->HealthPercentageOverride;
	ResLocation = InResFunction->ResLocation;
	static int ResIDCounter = 0;
	ResID = ResIDCounter++;
}