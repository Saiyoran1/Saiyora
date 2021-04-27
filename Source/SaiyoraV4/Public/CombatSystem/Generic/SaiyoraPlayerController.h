// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "SaiyoraPlayerController.generated.h"

/**
 * 
 */
class ASaiyoraGameState;

DECLARE_DYNAMIC_DELEGATE_OneParam(FPingCallback, float, NewPing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPingNotification, float, NewPing);

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

	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerFinalPingBounce(float const ServerTime);

	virtual void AcknowledgePossession(APawn* P) override;

	FPingNotification OnPingChanged;
	
protected:
	
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
    void OnClientPossess(APawn* NewPawn);

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetPlayerPing() const { return Ping; }

	UFUNCTION(BlueprintCallable, Category = "Ping")
	void SubscribeToPingChanged(FPingCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void UnsubscribeFromPingChanged(FPingCallback const& Callback);
};
