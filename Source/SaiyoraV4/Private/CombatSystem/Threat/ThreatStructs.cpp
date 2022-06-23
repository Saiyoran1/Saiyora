#include "Threat/ThreatStructs.h"
#include "Buff.h"
#include "ThreatHandler.h"

FThreatTarget::FThreatTarget(AActor* ThreatTarget, const float InitialThreat, const bool bFaded, UBuff* InitialFixate,
	UBuff* InitialBlind)
{
	Target = ThreatTarget;
	Threat = InitialThreat;
	Faded = bFaded;
	if (IsValid(InitialFixate))
	{
		Fixates.Add(InitialFixate);
	}
	if (IsValid(InitialBlind))
	{
		Blinds.Add(InitialBlind);
	}
}

bool FThreatTarget::LessThan(const FThreatTarget& Other) const
{
	//Check if we are blinded or faded.
	if (Blinds.Num() > 0 || Faded)
	{
		//If the other is also blinded or faded, order on threat.
		if (Other.Blinds.Num() > 0 || Other.Faded)
		{
			return Threat < Other.Threat;
		}
		//The other is not blinded or faded, and thus has higher target priority.
		return true;
	}
	//We are not blinded or faded, check if we have a fixate.
	if (Fixates.Num() > 0)
	{
		//Check if other has a fixate as well.
		if (Other.Fixates.Num() > 0)
		{
			//If other has a blind/fade overriding their fixate, so we have higher target priority.
			if (Other.Blinds.Num() > 0 || Other.Faded)
			{
				return false;
			}
			//Both have fixates and no blind/fade, determine based on threat.
			return Threat < Other.Threat;
		}
		//Other does not have a fixate, we have higher target priority.
		return false;
	}
	//We do not have blinds, fades, or fixates.
	if (Other.Blinds.Num() > 0 || Other.Faded)
	{
		//Other has blind or fade, so we have higher target priority.
		return false;
	}
	//Neither has blind or fade, and we don't have a fixate.
	if (Other.Fixates.Num() > 0)
	{
		//Other has a fixate, and we don't, so they have higher target priority.
		return true;
	}
	//Neither has any blinds, fades, or fixates. Order on threat.
	return Threat < Other.Threat;
}