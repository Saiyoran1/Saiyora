#include "AbilityCondition.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

bool FAbilityRangeCondition::CheckCondition(AActor* NPC, TSubclassOf<UCombatAbility> AbilityClass) const
{
	//TODO: Add checking with ability's parameters for range, rather than a hardcoded range value.
	//Can keep the hard-coded range value as an override value with an associated flag.
	const UThreatHandler* NPCThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(NPC);
	if (!IsValid(NPCThreat))
	{
		UE_LOG(LogTemp, Warning, TEXT("Checked range condition, no threat handler."));
		return false;
	}
	if (!IsValid(NPCThreat->GetCurrentTarget()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Checked range condition, no target."));
		return false;
	}
	const float Distance = FVector::Dist(NPCThreat->GetCurrentTarget()->GetActorLocation(), NPC->GetActorLocation());
	if (bUseAbilityRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("Checked range condition, using ability range."));
		//TODO: Get ability range from InstancedStruct AbilityParams (NYI).
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("Checked range condition, using override range."));
	return bCheckOutOfRange ? Distance > RangeOverride : Distance <= RangeOverride;
}
