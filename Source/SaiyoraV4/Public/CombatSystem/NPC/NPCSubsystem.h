#pragma once
#include "CoreMinimal.h"
#include "NPCStructs.h"
#include "WorldSubsystem.h"
#include "NPCSubsystem.generated.h"

class UNPCAbility;
class UCombatDebugOptions;

//World subsystem that acts as a centralized place to handle NPC behaviors like spreading out or distributing ability tokens.
UCLASS()
class SAIYORAV4_API UNPCSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY()
	UCombatDebugOptions* DebugOptions = nullptr;

#pragma region Claimed Locations
	
public:

	//Called when an NPC sets a location as its move goal. This stores it for other NPCs to avoid moving to the same location.
	void ClaimLocation(AActor* Actor, const FVector& Location);
	//Called when an NPC leaves combat, to free up its move goal location, so other NPCs don't have to avoid it anymore.
	void FreeLocation(AActor* Actor);
	//Helper function for calculating the "weight" of an NPC's goal location, based on how many other NPCs are moving to locations very close by.
	float GetScorePenaltyForLocation(const AActor* Actor, const FVector& Location) const;

private:

	UPROPERTY()
	TMap<AActor*, FVector> ClaimedLocations;

#pragma endregion
#pragma region Ability Tokens

public:

	void InitTokensForAbilityClass(const TSubclassOf<UNPCAbility> AbilityClass, const FAbilityTokenCallback& TokenCallback);
	//Request an ability token to use an ability. If bReservation is true, this will hold the token and only put it on a 1 frame cooldown if it is returned without being used.
	bool RequestAbilityToken(const UNPCAbility* Ability, const bool bReservation = false);
	//Returns a token so that it may be used again. Puts the token on cooldown. When freeing up a reservation that was never used, this cooldown is 1 frame.
	void ReturnAbilityToken(const UNPCAbility* Ability);
	//Checks whether there is either an available token, or a token already reserved for the querying ability.
	bool IsAbilityTokenAvailable(const UNPCAbility* Ability) const;

private:

	TMap<TSubclassOf<UNPCAbility>, FNPCAbilityTokens> AbilityTokens;
	UFUNCTION()
	void FinishTokenCooldown(const TSubclassOf<UNPCAbility> AbilityClass, const int TokenIndex);

#pragma endregion 
};
