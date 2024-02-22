#include "CombatDebugOptions.h"
#include "AbilityStructs.h"
#include "CombatAbility.h"
#include "NPCStructs.h"
#include "NPCAbility.h"

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

void UCombatDebugOptions::DisplayTokenInfo(const TMap<TSubclassOf<UNPCAbility>, FNPCAbilityTokens>& Tokens)
{
	for (const TTuple<TSubclassOf<UNPCAbility>, FNPCAbilityTokens>& TokenInfo : Tokens)
	{
		const FString AbilityName = TokenInfo.Key->GetName();
		int Available = 0;
		int InUse = 0;
		int OnCooldown = 0;
		for (const FNPCAbilityToken& Token : TokenInfo.Value.Tokens)
		{
			if (Token.bAvailable)
			{
				Available++;
			}
			else if (IsValid(Token.OwningInstance))
			{
				InUse++;
			}
			else
			{
				OnCooldown++;
			}
		}
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.0f, FColor::Blue, FString::Printf(
			L"%s: Available: %i, In Use: %i, On Cooldown: %i", *AbilityName, Available, InUse, OnCooldown));
	}
}
