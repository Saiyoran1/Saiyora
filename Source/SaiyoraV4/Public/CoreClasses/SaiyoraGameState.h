#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

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
	virtual void Tick(float DeltaSeconds) override;
	virtual float GetServerWorldTimeSeconds() const override { return WorldTime; }
	void UpdateClientWorldTime(const float ServerTime) { WorldTime = ServerTime; }

private:

	float WorldTime = 0.0f;

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

//Rewinding

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
};
