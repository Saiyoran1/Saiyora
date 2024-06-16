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

void UCombatDebugOptions::DisplayClaimedLocations(const TMap<AActor*, FVector>& Locations)
{
	for (const TTuple<AActor*, FVector>& Tuple : Locations)
	{
		if (IsValid(Tuple.Key))
		{
			DrawDebugSphere(Tuple.Key->GetWorld(), Tuple.Value + FVector::UpVector * 50.0f, 50.0f, 32, FColor::Cyan, false, -1);
			DrawDebugCircle(Tuple.Key->GetWorld(), Tuple.Value + FVector::UpVector * 50.0f, 200.0f, 32, FColor::Cyan, false, -1,
				0, 2, FVector(0, 1, 0),FVector(1, 0, 0), false);
			DrawDebugLine(Tuple.Key->GetWorld(), Tuple.Value, Tuple.Key->GetActorLocation() + FVector::UpVector * 50.0f, FColor::Cyan, false, -1);
		}
	}
}
