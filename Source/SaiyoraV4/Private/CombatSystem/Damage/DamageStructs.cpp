#include "DamageStructs.h"
#include "Buff.h"
#include "DamageBuffFunctions.h"

FPendingResurrection::FPendingResurrection(UPendingResurrectionFunction* InResFunction)
{
	Source = InResFunction;
	BuffClass = IsValid(InResFunction->GetOwningBuff()) ? InResFunction->GetOwningBuff()->GetClass() : nullptr;
	bOverrideHealth = InResFunction->bOverrideHealth;
	OverrideHealthPercent = InResFunction->HealthPercentageOverride;
	ResLocation = InResFunction->ResLocation;
	static int ResIDCounter = 0;
	ResID = ResIDCounter++;
}