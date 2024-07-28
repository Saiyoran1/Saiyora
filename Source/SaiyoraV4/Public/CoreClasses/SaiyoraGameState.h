#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

class USaiyoraGameInstance;
class ASaiyoraPlayerCharacter;

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
};
