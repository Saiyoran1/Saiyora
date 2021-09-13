// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraThreatFunctions.h"

#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "ThreatHandler.h"

FThreatEvent USaiyoraThreatFunctions::AddThreat(float const BaseThreat, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
                                                bool bIgnoreRestrictions, bool bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
	if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || BaseThreat <= 0.0f)
	{
		return Result;
	}
	if (AppliedBy->GetLocalRole() != ROLE_Authority)
	{
		return Result;
	}
	
	if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedTo);
	if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveThreat())
	{
		return Result;
	}
	//Target must either be alive, or not have a health component.
	UDamageHandler* TargetHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedTo);
	if (IsValid(TargetHealth) && TargetHealth->GetLifeStatus() != ELifeStatus::Alive)
	{
		return Result;
	}
	
	if (!AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return Result;
	}
	//Generator must either be alive, or not have a health component.
	UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->GetLifeStatus() != ELifeStatus::Alive)
	{
		return Result;
	}

	Result.AppliedTo = AppliedTo;
	Result.AppliedBy = IsValid(GeneratorComponent->GetMisdirectTarget()) ? GeneratorComponent->GetMisdirectTarget() : AppliedBy;
	if (Result.AppliedBy != AppliedBy)
	{
		//If we are misdirected to a different actor, that actor must also be alive or not have a health component.
		UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(Result.AppliedBy);
		if (IsValid(MisdirectHealth) && MisdirectHealth->GetLifeStatus() != ELifeStatus::Alive)
		{
			//If the misdirected actor is dead, threat will come from the original generator.
			Result.AppliedBy = AppliedBy;
		}
	}
	Result.Source = Source;
	Result.ThreatType = EThreatType::Absolute;
	Result.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedBy);
	Result.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedTo);
	Result.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		TArray<FCombatModifier> Mods;
		GeneratorComponent->GetOutgoingThreatMods(Mods, Result);
		TargetComponent->GetIncomingThreatMods(Mods, Result);
		if (SourceModifier.IsBound())
		{
			Mods.Add(SourceModifier.Execute(Result));
		}
		Result.Threat = FCombatModifier::ApplyModifiers(Mods, Result.Threat);
	}

	if (!bIgnoreRestrictions)
	{
		if (GeneratorComponent->CheckOutgoingThreatRestricted(Result) || TargetComponent->CheckIncomingThreatRestricted(Result))
		{
			return Result;
		}
	}

	TargetComponent->AddThreat(Result);

	return Result;
}
