#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

class USaiyoraGameInstance;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerAdded, ASaiyoraPlayerCharacter*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerRemoved);

UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

public:

	ASaiyoraGameState();
	virtual void Tick(float DeltaSeconds) override;

#pragma region Time

public:

	virtual double GetServerWorldTimeSeconds() const override { return WorldTime; }
	void UpdateClientWorldTime(const float ServerTime) { WorldTime = ServerTime; }

private:

	float WorldTime = 0.0f;

#pragma endregion 
#pragma region Player Handling

public:
	
	UFUNCTION(BlueprintPure, Category = "Players")
	void GetActivePlayers(TArray<ASaiyoraPlayerCharacter*>& OutActivePlayers) const { OutActivePlayers = ActivePlayers; }
	
	void InitPlayer(ASaiyoraPlayerCharacter* Player);

	UPROPERTY(BlueprintAssignable)
	FOnPlayerAdded OnPlayerAdded;
	UPROPERTY(BlueprintAssignable)
	FOnPlayerRemoved OnPlayerRemoved;

private:

	UPROPERTY()
	TArray<ASaiyoraPlayerCharacter*> ActivePlayers;

#pragma endregion 
};
