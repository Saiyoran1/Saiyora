#pragma once
#include "GameFramework/PlayerController.h"
#include "SaiyoraPlayerController.generated.h"

class APredictableProjectile;

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
	class ASaiyoraPlayerCharacter* GetSaiyoraPlayerCharacter() const { return PlayerCharacterRef; }
	UFUNCTION(BlueprintPure)
	class ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }

	UPROPERTY(BlueprintAssignable)
	FPingNotification OnPingChanged;

private:

	void RequestWorldTime();
	FTimerHandle WorldTimeRequestHandle;
	float Ping = 0.0f;

	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerHandleWorldTimeRequest(float const ClientTime, float const LastPing);
	UFUNCTION(Client, Unreliable)
	void ClientHandleWorldTimeReturn(float const ClientTime, float const ServerTime);
	UFUNCTION(Server, WithValidation, Unreliable)
	void ServerFinalPingBounce(float const ServerTime);

	UPROPERTY()
	class ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	class ASaiyoraPlayerCharacter* PlayerCharacterRef;
};
