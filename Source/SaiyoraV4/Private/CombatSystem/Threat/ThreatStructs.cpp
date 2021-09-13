// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatStructs.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

FThreatTarget::FThreatTarget()
{
	Target = nullptr;
	Threat = 0.0f;
	Fixates = 0;
	Blinds = 0;
}

FThreatTarget::FThreatTarget(AActor* ThreatTarget, float const InitialThreat, int32 const InitialFixates,
	int32 const InitialBlinds)
{
	Target = ThreatTarget;
	Threat = InitialThreat;
	Fixates = InitialFixates;
	Blinds = InitialBlinds;
}

bool FThreatTarget::LessThan(FThreatTarget const& Other) const
{
	//Check if we are blinded or faded.
	if (Blinds > 0 || ISaiyoraCombatInterface::Execute_GetThreatHandler(Target)->HasActiveFade())
	{
		//If the other is also blinded or faded, order on threat.
		if (Other.Blinds > 0 || ISaiyoraCombatInterface::Execute_GetThreatHandler(Other.Target)->HasActiveFade())
		{
			return Threat < Other.Threat;
		}
		//The other is not blinded or faded, and thus has higher target priority.
		return true;
	}
	//We are not blinded or faded, check if we have a fixate.
	if (Fixates > 0)
	{
		//Check if other has a fixate as well.
		if (Other.Fixates > 0)
		{
			//If other has a blind/fade overriding their fixate, so we have higher target priority.
			if (Other.Blinds > 0 || ISaiyoraCombatInterface::Execute_GetThreatHandler(Other.Target)->HasActiveFade())
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
	if (Other.Blinds > 0 || ISaiyoraCombatInterface::Execute_GetThreatHandler(Other.Target)->HasActiveFade())
	{
		//Other has blind or fade, so we have higher target priority.
		return false;
	}
	//Neither has blind or fade, and we don't have a fixate.
	if (Other.Fixates > 0)
	{
		//Other has a fixate, and we don't, so they have higher target priority.
		return true;
	}
	//Neither has any blinds, fades, or fixates. Order on threat.
	return Threat < Other.Threat;
}

