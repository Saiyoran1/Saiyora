#pragma once
#include "CoreMinimal.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/Character.h"
#include "SaiyoraPlayerCharacter.generated.h"

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerCharacter : public ACharacter, public ISaiyoraCombatInterface
{
	GENERATED_BODY()

public:
	
	ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer);
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual USaiyoraMovementComponent* GetCustomMovementComponent_Implementation() const override { return CustomMovementComponent; }
	virtual UFactionComponent* GetFactionComponent_Implementation() const override { return FactionComponent; }
	virtual UPlaneComponent* GetPlaneComponent_Implementation() const override { return PlaneComponent; }
	virtual UDamageHandler* GetDamageHandler_Implementation() const override { return DamageHandler; }
	virtual UThreatHandler* GetThreatHandler_Implementation() const override { return ThreatHandler; }
	virtual UBuffHandler* GetBuffHandler_Implementation() const override { return BuffHandler; }
	virtual UStatHandler* GetStatHandler_Implementation() const override { return StatHandler; }
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const override { return CcHandler; }
	virtual UAbilityComponent* GetAbilityComponent_Implementation() const override { return AbilityComponent; }
	virtual UResourceHandler* GetResourceHandler_Implementation() const override { return ResourceHandler; }

	UFUNCTION(BlueprintPure)
	class ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }
	UFUNCTION(BlueprintPure)
	class ASaiyoraPlayerController* GetSaiyoraPlayerController() const { return PlayerControllerRef; }

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void CreateUserInterface();

private:

	bool bInitialized = false;
	void InitializeCharacter();
	
	UPROPERTY()
	USaiyoraMovementComponent* CustomMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UFactionComponent* FactionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UPlaneComponent* PlaneComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UDamageHandler* DamageHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UThreatHandler* ThreatHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBuffHandler* BuffHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStatHandler* StatHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCrowdControlHandler* CcHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UAbilityComponent* AbilityComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UResourceHandler* ResourceHandler;

	UPROPERTY()
	class ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	class ASaiyoraPlayerController* PlayerControllerRef;
};
