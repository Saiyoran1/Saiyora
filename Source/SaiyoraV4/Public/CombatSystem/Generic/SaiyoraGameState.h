// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SaiyoraPlayerController.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

/**
 * 
 */
UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

private:

	float WorldTime = 0.0f;
	void UpdateClientWorldTime(float const ServerTime);

public:

	ASaiyoraGameState();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "World Status")
	float GetWorldTime() const { return WorldTime; }

	virtual float GetServerWorldTimeSeconds() const override;

	friend void ASaiyoraPlayerController::ClientHandleWorldTimeReturn_Implementation(float const ClientTime, float const ServerTime);

protected:

	virtual void Tick(float DeltaSeconds) override;
};
