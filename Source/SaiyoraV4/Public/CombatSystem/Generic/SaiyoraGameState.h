// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "SaiyoraPlayerController.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

private:
	float WorldTime = 0.0f;
	void UpdateClientWorldTime(float const ServerTime);
public:
	ASaiyoraGameState();
	virtual float GetServerWorldTimeSeconds() const override;
	friend void ASaiyoraPlayerController::ClientHandleWorldTimeReturn_Implementation(float const ClientTime, float const ServerTime);
	//TODO: Add a place to track local player's plane, so that objects can check for xplane during rendering.
protected:
	virtual void Tick(float DeltaSeconds) override;
};
