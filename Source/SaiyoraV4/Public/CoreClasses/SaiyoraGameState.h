#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

class USaiyoraGameInstance;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerAddRemoveNotification, ASaiyoraPlayerCharacter*, Player);

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
	void RemovePlayer(ASaiyoraPlayerCharacter* Player);

	UPROPERTY(BlueprintAssignable)
	FPlayerAddRemoveNotification OnPlayerAdded;
	UPROPERTY(BlueprintAssignable)
	FPlayerAddRemoveNotification OnPlayerRemoved;

private:

	UPROPERTY()
	TArray<ASaiyoraPlayerCharacter*> ActivePlayers;

#pragma endregion 
};
