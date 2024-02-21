#include "NPCAbility.h"
#include "AbilityComponent.h"
#include "ThreatHandler.h"

void UNPCAbility::PostInitializeAbility_Implementation()
{
	Super::PostInitializeAbility_Implementation();

	if (GetHandler()->GetOwner()->HasAuthority())
	{
		
	}
}

void UNPCAbility::AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons)
{
	Super::AdditionalCastableUpdate(AdditionalFailReasons);


}