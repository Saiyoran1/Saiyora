#include "TargetContexts.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

AActor* FNPCTargetContext_HighestThreat::GetBestTarget(const AActor* Querier) const
{
	if (!IsValid(Querier) || !Querier->Implements<USaiyoraCombatInterface>())
	{
		return nullptr;
	}
	const UThreatHandler* QuerierThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Querier);
	if (!IsValid(QuerierThreat))
	{
		return nullptr;
	}
	return QuerierThreat->GetCurrentTarget();
}

AActor* FNPCTargetContext_ClosestTarget::GetBestTarget(const AActor* Querier) const
{
	if (!IsValid(Querier) || !Querier->Implements<USaiyoraCombatInterface>())
	{
		return nullptr;
	}
	const UThreatHandler* QuerierThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Querier);
	if (!IsValid(QuerierThreat))
	{
		return nullptr;
	}
	
	TArray<AActor*> Targets;
	QuerierThreat->GetActorsInThreatTable(Targets);
	float BestDistSq = FLT_MAX;
	AActor* BestTarget = nullptr;
	for (AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Querier->GetActorLocation(), Target->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Target;
		}
	}

	return BestTarget;
}