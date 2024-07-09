#include "ChoiceRequirements.h"
#include "SaiyoraCombatInterface.h"
#include "TargetContexts.h"
#include "ThreatHandler.h"

#pragma region Choice Requirement

void FNPCChoiceRequirement::Init(FNPCCombatChoice* Choice, AActor* NPC)
{
	OwningChoice = Choice;
	OwningNPC = NPC;
	OwningChoice = Choice;
	SetupRequirement();
}

#pragma endregion 
#pragma region Time In Combat

bool FNPCCR_TimeInCombat::IsMet() const
{
	if (!IsValid(ThreatHandler))
	{
		return false;
	}
	const float CombatTime = ThreatHandler->GetCombatTime();
	return bMetAfterTimeReached ? CombatTime > TimeRequirement : CombatTime < TimeRequirement;
}

void FNPCCR_TimeInCombat::SetupRequirement()
{
	const AActor* Owner = GetOwningNPC();
	if (!IsValid(Owner) || !Owner->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	ThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Owner);
}

#pragma endregion
#pragma region Range Of Target

bool FNPCCR_RangeOfTarget::IsMet() const
{
	const FNPCTargetContext* Context = TargetContext.GetPtr<FNPCTargetContext>();
	if (!Context)
	{
		return false;
	}
	const AActor* Target = Context->GetBestTarget(GetOwningNPC());
	if (!IsValid(Target))
	{
		return false;
	}
	const float DistSqToTarget = bIncludeZDistance ? FVector::DistSquared(Target->GetActorLocation(), GetOwningNPC()->GetActorLocation())
		: FVector::DistSquared2D(Target->GetActorLocation(), GetOwningNPC()->GetActorLocation());
	const float ClampedRangeSq = FMath::Square(FMath::Max(0.0f, Range));
	return bWithinRange ? DistSqToTarget < ClampedRangeSq : DistSqToTarget >= ClampedRangeSq;
}

#pragma endregion 