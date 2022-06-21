#pragma once
#include "GameFramework/PlayerController.h"
#include "SaiyoraPlayerController.generated.h"

class APredictableProjectile;
class ASaiyoraGameState;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPingNotification, float, NewPing);

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetPlayerPing() const { return Ping; }

	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerCharacter* GetSaiyoraPlayerCharacter() const { return PlayerCharacterRef; }
	UFUNCTION(BlueprintPure)
	ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }

	UPROPERTY(BlueprintAssignable)
	FPingNotification OnPingChanged;

private:

	void RequestWorldTime();
	FTimerHandle WorldTimeRequestHandle;
	float Ping = 0.0f;

	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerHandleWorldTimeRequest(const float ClientTime, const float LastPing);
	UFUNCTION(Client, Unreliable)
	void ClientHandleWorldTimeReturn(const float ClientTime, const float ServerTime);
	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerFinalPingBounce(const float ServerTime);

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerCharacterRef;
};
