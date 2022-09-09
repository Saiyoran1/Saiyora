#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "AbilityStructs.h"
#include "CombatEnums.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/Character.h"
#include "SaiyoraPlayerCharacter.generated.h"

class ASaiyoraGameState;
class ASaiyoraPlayerController;
class UCombatAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMappingChanged, const ESaiyoraPlane, Plane, const int32, MappingID, UCombatAbility*, Ability);

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerCharacter : public ACharacter, public ISaiyoraCombatInterface
{
	GENERATED_BODY()

public:
	
	ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer);
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
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
	ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }
	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerController* GetSaiyoraPlayerController() const { return PlayerControllerRef; }

	UPROPERTY(BlueprintAssignable)
	FOnMappingChanged OnMappingChanged;
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetAbilityMappings(TMap<int32, UCombatAbility*>& AncientMappings, TMap<int32, UCombatAbility*>& ModernMappings) const { AncientMappings = AncientAbilityMappings; ModernMappings = ModernAbilityMappings; }

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void CreateUserInterface();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void AbilityInput(const int32 InputNum, const bool bPressed);

private:

	bool bInitialized = false;
	void InitializeCharacter();
	
	UPROPERTY()
	USaiyoraMovementComponent* CustomMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UFactionComponent* FactionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UPlaneComponent* PlaneComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UDamageHandler* DamageHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UThreatHandler* ThreatHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBuffHandler* BuffHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStatHandler* StatHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCrowdControlHandler* CcHandler;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAbilityComponent* AbilityComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UResourceHandler* ResourceHandler;

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	ASaiyoraPlayerController* PlayerControllerRef;

	static const int32 MAXABILITYBINDS;
	void SetupAbilityMappings();
	TMap<int32, UCombatAbility*> ModernAbilityMappings;
	TMap<int32, UCombatAbility*> AncientAbilityMappings;
	UFUNCTION()
	void AddAbilityMapping(UCombatAbility* NewAbility);
	UFUNCTION()
	void RemoveAbilityMapping(UCombatAbility* RemovedAbility);
	
	static const float ABILITYQUEWINDOW;
	UFUNCTION()
	void UpdateQueueOnGlobalEnd(const FGlobalCooldown& OldGlobalCooldown, const FGlobalCooldown& NewGlobalCooldown);
	UFUNCTION()
	void UpdateQueueOnCastEnd(const FCastingState& OldState, const FCastingState& NewState);
	UFUNCTION()
	void ClearQueueAndAutoFireOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
	UFUNCTION()
	void UseAbilityFromQueue(const TSubclassOf<UCombatAbility> AbilityClass);
	EQueueStatus QueueStatus = EQueueStatus::Empty;
	TSubclassOf<UCombatAbility> QueuedAbility;
	bool bUsingAbilityFromQueue = false;
	bool TryQueueAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	FTimerHandle QueueExpirationHandle;
	UFUNCTION()
	void ExpireQueue();

	UPROPERTY()
	UCombatAbility* AutomaticInputAbility = nullptr;
};