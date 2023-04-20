#include "AbilityChoice.h"
#include "NPCBehavior.h"

void UAbilityChoice::InitializeChoice(UNPCBehavior* BehaviorComp)
{
	if (!IsValid(BehaviorComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid behavior component reference passed to ability choice."));
		return;
	}
	BehaviorComponentRef = BehaviorComp;
	CurrentScore = FModifiableFloat(DefaultScore, true, true, 0.0f);
}
