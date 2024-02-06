#include "CombatDebugOptions.h"
#include "AbilityStructs.h"
#include "CombatAbility.h"

void UCombatDebugOptions::LogAbilityEvent(const AActor* Actor, const FAbilityEvent& Event)
{
	FString LogString = FString::Printf(TEXT("%s: Ability %s cast result was %s. "),
		IsValid(Actor) ? *Actor->GetName() : TEXT("null"), IsValid(Event.Ability) ? *Event.Ability->GetName() : TEXT("null"), *UEnum::GetDisplayValueAsText(Event.ActionTaken).ToString());
	
	if (Event.FailReasons.Num() > 0)
	{
		LogString += FString::Printf(TEXT("Fail reasons: "));
		for (int i = 0; i < Event.FailReasons.Num(); i++)
		{
			LogString += UEnum::GetDisplayValueAsText(Event.FailReasons[i]).ToString();
			if (i < Event.FailReasons.Num() - 1)
			{
				LogString += TEXT(", ");
			}
			else
			{
				LogString += TEXT(". ");
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("%s"), *LogString);
}
