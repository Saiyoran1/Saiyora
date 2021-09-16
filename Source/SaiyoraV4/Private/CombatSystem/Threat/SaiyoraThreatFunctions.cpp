// Fill out your copyright notice in the Description page of Project Settings.

#include "SaiyoraThreatFunctions.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

FThreatEvent USaiyoraThreatFunctions::AddThreat(float const BaseThreat, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
                                                bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	UThreatHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedTo);
	if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveThreat())
	{
		FThreatEvent const Failure;
		return Failure;
	}
	return TargetComponent->AddThreat(EThreatType::Absolute, BaseThreat, AppliedBy, Source, bIgnoreRestrictions, bIgnoreModifiers, SourceModifier);
}
