#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "AbilityStructs.h"
#include "CombatEnums.h"
#include "SaiyoraCombatInterface.h"
#include "SpecializationStructs.h"
#include "ThreatStructs.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "SaiyoraPlayerCharacter.generated.h"

class ASaiyoraGameState;
class ASaiyoraPlayerController;
class UCombatAbility;
class APredictableProjectile;
class UFireWeapon;
class UStopFiring;
class AWeapon;
class UReload;
class UAncientSpecialization;
class UAncientTalent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMappingChanged, const ESaiyoraPlane, Plane, const int32, MappingID, UCombatAbility*, Ability);

USTRUCT()
struct FPredictedTickProjectiles
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<int32, APredictableProjectile*> Projectiles;
};

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerCharacter : public ACharacter, public ISaiyoraCombatInterface
{
	GENERATED_BODY()

//Setup

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

	UFUNCTION(BlueprintPure)
	ASaiyoraGameState* GetSaiyoraGameState() const { return GameStateRef; }
	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerController* GetSaiyoraPlayerController() const { return PlayerControllerRef; }

	void NotifyEnemyCombatChanged(AActor* Enemy, const bool bInCombat) { OnEnemyCombatChanged.Broadcast(Enemy, bInCombat); }

	UPROPERTY(BlueprintAssignable)
	FActorCombatNotification OnEnemyCombatChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

protected:
	
	UFUNCTION(BlueprintImplementableEvent)
	void CreateUserInterface();

private:

	bool bInitialized = false;
	void InitializeCharacter();

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

	UFUNCTION()
	void InputPlaneSwap() { Server_PlaneSwapInput(); }
	UFUNCTION(Server, Reliable)
	void Server_PlaneSwapInput();
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

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	ASaiyoraPlayerController* PlayerControllerRef;
	
//Saiyora Combat Interface

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

//Ability Mappings

public:

	UPROPERTY(BlueprintAssignable)
	FOnMappingChanged OnMappingChanged;
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetAbilityMappings(TMap<int32, UCombatAbility*>& AncientMappings, TMap<int32, UCombatAbility*>& ModernMappings) const { AncientMappings = AncientAbilityMappings; ModernMappings = ModernAbilityMappings; }

private:
	
	static constexpr int32 MaxAbilityBinds = 6;
	void SetupAbilityMappings();
	TMap<int32, UCombatAbility*> ModernAbilityMappings;
	TMap<int32, UCombatAbility*> AncientAbilityMappings;
	UFUNCTION()
	void AddAbilityMapping(UCombatAbility* NewAbility);
	UFUNCTION()
	void RemoveAbilityMapping(UCombatAbility* RemovedAbility);

//Weapon Handling

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

//Ability Input

protected:

	UFUNCTION(BlueprintCallable, Category = "Input")
	void AbilityInput(const int32 InputNum, const bool bPressed);

private:
	
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

//Collision

private:

	UFUNCTION()
	void HandleBeginXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void HandleEndXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UPROPERTY()
	TSet<UPrimitiveComponent*> XPlaneOverlaps;

//Projectiles

public:

	void RegisterClientProjectile(APredictableProjectile* Projectile);
	void ReplaceProjectile(APredictableProjectile* AuthProjectile);

	int32 GetNewProjectileID(const FPredictedTick& Tick);

	UFUNCTION(Client, Reliable)
	void ClientNotifyFailedProjectileSpawn(const FPredictedTick& Tick, const int32 ProjectileID);

private:
	
	TMap<FPredictedTick, FPredictedTickProjectiles> PredictedProjectiles;
	TMap<FPredictedTick, int32> ProjectileIDs;

//Specialization

public:

	UFUNCTION(BlueprintPure, Category = "Specialization")
	UAncientSpecialization* GetAncientSpecialization() const { return AncientSpec; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void SetAncientSpecialization(const TSubclassOf<UAncientSpecialization> NewSpec);

	UPROPERTY(BlueprintAssignable)
	FAncientSpecChangeNotification OnAncientSpecChanged;

	UFUNCTION(BlueprintPure, Category = "Specialization")
	UModernSpecialization* GetModernSpecialization() const { return ModernSpec; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void SetModernSpecialization(const TSubclassOf<UModernSpecialization> NewSpec);

	UPROPERTY(BlueprintAssignable)
	FModernSpecChangeNotification OnModernSpecChanged;

private:

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
};
