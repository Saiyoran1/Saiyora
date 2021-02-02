// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "SaiyoraPlayerController.generated.h"

/**
 * 
 */
class ASaiyoraGameState;

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	ASaiyoraGameState* GameStateRef;

	void RequestWorldTime();
	FTimerHandle WorldTimeRequestHandle;
	float Ping = 0.0f;

	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerHandleWorldTimeRequest(float const ClientTime, float const LastPing);

	UFUNCTION(Client, Unreliable)
	void ClientHandleWorldTimeReturn(float const ClientTime, float const ServerTime);

	virtual void AcknowledgePossession(APawn* P) override;
	
protected:
	
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
    void OnClientPossess(APawn* NewPawn);

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetPlayerPing() const { return Ping; }
};
