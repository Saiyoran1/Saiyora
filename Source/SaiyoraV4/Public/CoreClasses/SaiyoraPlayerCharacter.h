#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "AbilityStructs.h"
#include "CombatEnums.h"
#include "DamageStructs.h"
#include "SaiyoraCombatInterface.h"
#include "SpecializationStructs.h"
#include "ThreatStructs.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "SaiyoraPlayerCharacter.generated.h"

class UPlayerHUD;
class USaiyoraErrorMessage;
class USaiyoraUIDataAsset;
class ASaiyoraGameState;
class ASaiyoraPlayerController;
class UCombatAbility;
class UFireWeapon;
class UStopFiring;
class AWeapon;
class UReload;
class UAncientSpecialization;
class UAncientTalent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMappingChanged, const ESaiyoraPlane, Plane, const int32, MappingID, TSubclassOf<UCombatAbility>, Ability);

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerCharacter : public ACharacter, public ISaiyoraCombatInterface
{
	GENERATED_BODY()

#pragma region Initialization

public:
	
	ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer);
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	bool bInitialized = false;
	void InitializeCharacter();

#pragma endregion 
#pragma region Core

public:

	UFUNCTION(BlueprintPure)
	ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }
	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerController* GetSaiyoraPlayerController() const { return PlayerControllerRef; }

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	ASaiyoraPlayerController* PlayerControllerRef;

#pragma endregion 
#pragma region User Interface

public:

	UFUNCTION(BlueprintCallable)
	void DisplayErrorMessage(const FText& Message, const float Duration) { Client_DisplayErrorMessage(Message, Duration); }
	
	void NotifyEnemyCombatChanged(AActor* Enemy, const bool bInCombat) { OnEnemyCombatChanged.Broadcast(Enemy, bInCombat); }
	UPROPERTY(BlueprintAssignable)
	FActorCombatNotification OnEnemyCombatChanged;

	UPlayerHUD* GetPlayerHUD() const { return PlayerHUD; }

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void CreateUserInterface();
	void InitUserInterface();

private:

	UFUNCTION()
	void ShowExtraInfo();
	UFUNCTION()
	void HideExtraInfo();
	UPROPERTY()
	UPlayerHUD* PlayerHUD = nullptr;
	UPROPERTY()
	USaiyoraErrorMessage* ErrorWidget;
	UFUNCTION(Client, Unreliable)
	void Client_DisplayErrorMessage(const FText& Message, const float Duration);

#pragma endregion 
#pragma region Movement and Looking

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

private:

	UFUNCTION()
	void InputJump() { Jump(); }
	UFUNCTION()
	void InputStartCrouch() { Crouch(); }
	UFUNCTION()
	void InputStopCrouch() { UnCrouch(); }
	UFUNCTION()
	void InputMoveForward(const float AxisValue) { AddMovementInput(GetActorForwardVector(), AxisValue); }
	UFUNCTION()
	void InputMoveRight(const float AxisValue) { AddMovementInput(GetActorRightVector(), AxisValue); }
	UFUNCTION()
	void InputLookHorizontal(const float AxisValue) { AddControllerYawInput(AxisValue); }
	UFUNCTION()
	void InputLookVertical(const float AxisValue) { AddControllerPitchInput(AxisValue * -1.0f); }

#pragma endregion 
#pragma region Saiyora Combat Interface

public:

	virtual USaiyoraMovementComponent* GetCustomMovementComponent_Implementation() const override { return CustomMovementComponent; }
	virtual UCombatStatusComponent* GetCombatStatusComponent_Implementation() const override { return CombatStatusComponent; }
	virtual UDamageHandler* GetDamageHandler_Implementation() const override { return DamageHandler; }
	virtual UThreatHandler* GetThreatHandler_Implementation() const override { return ThreatHandler; }
	virtual UBuffHandler* GetBuffHandler_Implementation() const override { return BuffHandler; }
	virtual UStatHandler* GetStatHandler_Implementation() const override { return StatHandler; }
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const override { return CcHandler; }
	virtual UAbilityComponent* GetAbilityComponent_Implementation() const override { return AbilityComponent; }
	virtual UResourceHandler* GetResourceHandler_Implementation() const override { return ResourceHandler; }

private:

	UPROPERTY()
	USaiyoraMovementComponent* CustomMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatStatusComponent* CombatStatusComponent;
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

#pragma endregion 
#pragma region Weapon
	
public:

	void SetWeapon(AWeapon* NewWeapon) { Weapon = NewWeapon; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetWeapon() const { return Weapon; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	UReload* GetReloadAbility() const { return ReloadAbility; }

private:

	UPROPERTY()
	UFireWeapon* FireWeaponAbility;
	UPROPERTY()
	AWeapon* Weapon;
	UPROPERTY()
	UStopFiring* StopFiringAbility;
	UPROPERTY()
	UReload* ReloadAbility;
	
#pragma endregion
#pragma region Ability Mappings

public:

	static constexpr int32 MaxAbilityBinds = 6;

	UPROPERTY(BlueprintAssignable)
	FOnMappingChanged OnMappingChanged;
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetAbilityMappings(TMap<int32, TSubclassOf<UCombatAbility>>& AncientMappings, TMap<int32, TSubclassOf<UCombatAbility>>& ModernMappings) const
	{
		AncientMappings = AncientAbilityMappings; ModernMappings = ModernAbilityMappings;
	}
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetAncientMappings(TMap<int32, TSubclassOf<UCombatAbility>>& AncientMappings) const { AncientMappings = AncientAbilityMappings; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetModernMappings(TMap<int32, TSubclassOf<UCombatAbility>>& ModernMappings) const { ModernMappings = ModernAbilityMappings; }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SetAbilityMapping(const ESaiyoraPlane Plane, const int32 BindIndex, const TSubclassOf<UCombatAbility> AbilityClass);
	UFUNCTION(BlueprintPure, Category = "Abilities")
	TSubclassOf<UCombatAbility> GetAbilityMapping(const ESaiyoraPlane Plane, const int32 BindIndex) const;

private:
	
	void SetupAbilityMappings();
	TMap<int32, TSubclassOf<UCombatAbility>> ModernAbilityMappings;
	TMap<int32, TSubclassOf<UCombatAbility>> AncientAbilityMappings;
	UFUNCTION()
	void OnAbilityAdded(UCombatAbility* NewAbility);
	UFUNCTION()
	void OnAbilityRemoved(UCombatAbility* RemovedAbility);

#pragma endregion 
#pragma region Ability Input

protected:

	UFUNCTION(BlueprintCallable, Category = "Input")
	void AbilityInput(const int32 InputNum, const bool bPressed);

private:

	UFUNCTION()
	void InputReload();
	UFUNCTION()
	void InputStartAbility0() { AbilityInput(0, true); }
	UFUNCTION()
	void InputStopAbility0() { AbilityInput(0, false); }
	UFUNCTION()
	void InputStartAbility1() { AbilityInput(1, true); }
	UFUNCTION()
	void InputStopAbility1() { AbilityInput(1, false); }
	UFUNCTION()
	void InputStartAbility2() { AbilityInput(2, true); }
	UFUNCTION()
	void InputStopAbility2() { AbilityInput(2, false); }
	UFUNCTION()
	void InputStartAbility3() { AbilityInput(3, true); }
	UFUNCTION()
	void InputStopAbility3() { AbilityInput(3, false); }
	UFUNCTION()
	void InputStartAbility4() { AbilityInput(4, true); }
	UFUNCTION()
	void InputStopAbility4() { AbilityInput(4, false); }
	UFUNCTION()
	void InputStartAbility5() { AbilityInput(5, true); }
	UFUNCTION()
	void InputStopAbility5() { AbilityInput(5, false); }
	
	static constexpr float AbilityQueueWindow = 0.2f;
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
	
#pragma endregion 
#pragma region Specialization

public:

	UFUNCTION(BlueprintPure, Category = "Specialization")
	UPlayerAbilityData* GetPlayerAbilityData() const { return PlayerAbilityData; }

	UFUNCTION(BlueprintPure, Category = "Specialization")
	UAncientSpecialization* GetAncientSpecialization() const { return AncientSpec; }
	UPROPERTY(BlueprintAssignable)
	FAncientSpecChangeNotification OnAncientSpecChanged;

	UFUNCTION(BlueprintPure, Category = "Specialization")
	UModernSpecialization* GetModernSpecialization() const { return ModernSpec; }
	UPROPERTY(BlueprintAssignable)
	FModernSpecChangeNotification OnModernSpecChanged;

	void ApplyNewAncientLayout(const FAncientSpecLayout& NewLayout);
	UFUNCTION(Server, Reliable)
	void Server_UpdateAncientSpecAndTalents(TSubclassOf<UAncientSpecialization> NewSpec, const TArray<FAncientTalentSelection>& TalentSelections);

	void ApplyNewModernLayout(const FModernSpecLayout& NewLayout);
	UFUNCTION(Server, Reliable)
	void Server_UpdateModernSpecAndTalents(TSubclassOf<UModernSpecialization> NewSpec, const TArray<FModernTalentChoice>& TalentSelections);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	UPlayerAbilityData* PlayerAbilityData = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_AncientSpec)
	UAncientSpecialization* AncientSpec;
	UFUNCTION()
	void OnRep_AncientSpec(UAncientSpecialization* PreviousSpec);
	UPROPERTY()
	UAncientSpecialization* RecentlyUnlearnedAncientSpec;
	UFUNCTION()
	void CleanupOldAncientSpecialization() { RecentlyUnlearnedAncientSpec = nullptr; }

	UPROPERTY(ReplicatedUsing = OnRep_ModernSpec)
	UModernSpecialization* ModernSpec;
	UFUNCTION()
	void OnRep_ModernSpec(UModernSpecialization* PreviousSpec);
	UPROPERTY()
	UModernSpecialization* RecentlyUnlearnedModernSpec;
	UFUNCTION()
	void CleanupOldModernSpecialization() { RecentlyUnlearnedModernSpec = nullptr; }

#pragma endregion 
#pragma region Plane

private:

	UFUNCTION()
	void InputPlaneSwap() { Server_PlaneSwapInput(); }
	UFUNCTION(Server, Reliable)
	void Server_PlaneSwapInput();
	UFUNCTION()
	void HandleBeginXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void HandleEndXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UPROPERTY()
	TSet<UPrimitiveComponent*> XPlaneOverlaps;

#pragma endregion 
};
