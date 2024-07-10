#pragma once
#include "CoreMinimal.h"
#include "NPCAbility.h"
#include "NPCStructs.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

class USaiyoraGameInstance;
class UHitbox;
class ASaiyoraPlayerCharacter;

USTRUCT()
struct FRewindRecord
{
	GENERATED_BODY()

	TArray<TTuple<float, FTransform>> Transforms;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerAdded, const ASaiyoraPlayerCharacter*, Player);

UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

public:

	ASaiyoraGameState();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual double GetServerWorldTimeSeconds() const override { return WorldTime; }
	void UpdateClientWorldTime(const float ServerTime) { WorldTime = ServerTime; }

private:

	float WorldTime = 0.0f;
	UPROPERTY()
	USaiyoraGameInstance* GameInstanceRef = nullptr;

//Player Handling

public:

	void InitPlayer(ASaiyoraPlayerCharacter* Player);
	UPROPERTY(BlueprintAssignable)
	FOnPlayerAdded OnPlayerAdded;
	UFUNCTION(BlueprintPure, Category = "Players")
	void GetActivePlayers(TArray<ASaiyoraPlayerCharacter*>& OutActivePlayers) const { OutActivePlayers = ActivePlayers; }

private:

	UPROPERTY()
	TArray<ASaiyoraPlayerCharacter*> ActivePlayers;

#pragma region Hitbox Rewinding

public:

	void RegisterNewHitbox(UHitbox* Hitbox);
	FTransform RewindHitbox(UHitbox* Hitbox, const float Timestamp);

private:

	static constexpr float SnapshotInterval = 0.03f;
	static constexpr float MaxLagCompensation = 0.2f;
	TMap<UHitbox*, FRewindRecord> Snapshots;
	UFUNCTION()
	void CreateSnapshot();
	FTimerHandle SnapshotHandle;

#pragma endregion 
#pragma region NPC Ability Tokens

public:

	void InitTokensForAbilityClass(const TSubclassOf<UNPCAbility> AbilityClass, const FAbilityTokenCallback& TokenCallback);
	//Request an ability token to use an ability. If bReservation is true, this will hold the token and only put it on a 1 frame cooldown if it is returned without being used.
	bool RequestAbilityToken(const UNPCAbility* Ability, const bool bReservation = false);
	//Returns a token so that it may be used again. Puts the token on cooldown. When freeing up a reservation that was never used, this cooldown is 1 frame.
	void ReturnAbilityToken(const UNPCAbility* Ability);
	//Checks whether there is either an available token, or a token already reserved for the querying ability.
	bool IsAbilityTokenAvailable(const UNPCAbility* Ability) const;

private:

	TMap<TSubclassOf<UNPCAbility>, FNPCAbilityTokens> Tokens;
	UFUNCTION()
	void FinishTokenCooldown(const TSubclassOf<UNPCAbility> AbilityClass, const int TokenIndex);

#pragma endregion
#pragma region NPC Location Claims

public:
	
	void ClaimLocation(AActor* Actor, const FVector& Location);
	void FreeLocation(AActor* Actor);
	float GetScorePenaltyForLocation(const AActor* Actor, const FVector& Location) const;

private:

	UPROPERTY()
	TMap<AActor*, FVector> ClaimedLocations;

#pragma endregion 
};
