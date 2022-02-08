#pragma once
#include "CoreMinimal.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "MovementStructs.h"
#include "SaiyoraRootMotionHandler.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilityComponent.h"
#include "BuffStructs.h"
#include "SaiyoraMovementComponent.generated.h"

class UBuffHandler;
class UCrowdControlHandler;
class UDamageHandler;
class AGameState;

UCLASS(BlueprintType)
class SAIYORAV4_API USaiyoraMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

//Custom Move Structs

private:
	
	class FSavedMove_Saiyora : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void PrepMoveFor(ACharacter* C) override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

		uint8 bSavedWantsCustomMove : 1;
		FClientPendingCustomMove SavedPendingCustomMove;
	};

	class FNetworkPredictionData_Client_Saiyora : public FNetworkPredictionData_Client_Character
	{
	public:
		typedef FNetworkPredictionData_Client_Character Super;
		FNetworkPredictionData_Client_Saiyora(const UCharacterMovementComponent& ClientMovement);
		virtual FSavedMovePtr AllocateNewMove() override;
	};
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	struct FSaiyoraNetworkMoveData : public FCharacterNetworkMoveData
	{
		typedef FCharacterNetworkMoveData Super;
		FAbilityRequest CustomMoveAbilityRequest;
		virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
		virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
	};

	struct FSaiyoraNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
	{
		typedef FCharacterNetworkMoveDataContainer Super;
		FSaiyoraNetworkMoveDataContainer();
		FSaiyoraNetworkMoveData CustomDefaultMoveData[3];
	};

//Setup

public:

	static FGameplayTag GenericExternalMovementTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Movement")), false); }
	static FGameplayTag GenericMovementRestrictionTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Movement.Restriction")), false); }
	
	USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer);
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
	UAbilityComponent* GetOwnerAbilityHandler() const { return OwnerAbilityHandler; }
	AGameState* GetGameStateRef() const { return GameStateRef; }
	
private:
	
	UPROPERTY()
	UAbilityComponent* OwnerAbilityHandler = nullptr;
	UPROPERTY()
	UCrowdControlHandler* OwnerCcHandler = nullptr;
	UPROPERTY()
	UDamageHandler* OwnerDamageHandler = nullptr;
	UPROPERTY()
	UStatHandler* OwnerStatHandler = nullptr;
	UPROPERTY()
	UBuffHandler* OwnerBuffHandler = nullptr;
	UPROPERTY()
	AGameState* GameStateRef = nullptr;

//Core Movement

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	static const float MaxPingDelay;
	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;
	uint8 bWantsCustomMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	TMap<int32, bool> CompletedCastStatus;
	TSet<FPredictedTick> ServerCompletedMovementIDs;
	TArray<UObject*> CurrentTickServerCustomMoveSources;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	bool ApplyCustomMove(FCustomMoveParams const& CustomMove, UObject* Source);
	void SetupCustomMovementPrediction(UCombatAbility* Source, FCustomMoveParams const& CustomMove);
	FAbilityCallback OnPredictedAbility;
	UFUNCTION()
	void OnCustomMoveCastPredicted(FAbilityEvent const& Event);
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMoveNoOwner(FCustomMoveParams const& CustomMove);
	UFUNCTION(Client, Unreliable)
	void Client_ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	UFUNCTION()
	void DelayedCustomMoveExecution(FCustomMoveParams CustomMove);
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	void CustomMoveFromFlag();
	void ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	FAbilityMispredictionCallback OnMispredict;
	UFUNCTION()
	void AbilityMispredicted(int32 const PredictionID);

//Custom Moves
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	bool TeleportToLocation(UObject* Source, FVector const& Target, FRotator const& DesiredRotation, bool const bStopMovement, bool const bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	bool LaunchPlayer(UObject* Source, FVector const& LaunchVector, bool const bStopMovement, bool const bIgnoreRestrictions);
	
private:
	
	void ExecuteTeleportToLocation(FCustomMoveParams const& CustomMove);
	void ExecuteLaunchPlayer(FCustomMoveParams const& CustomMove);

//Restrictions
	
private:

	UPROPERTY()
	TArray<UBuff*> MovementRestrictions;
	UPROPERTY(ReplicatedUsing=OnRep_MovementRestricted)
	bool bMovementRestricted = false;
	UFUNCTION()
	void OnRep_MovementRestricted();
	virtual bool CanAttemptJump() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual FVector ConsumeInputVector() override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;
	FLifeStatusCallback OnDeath;
	UFUNCTION()
	void StopMotionOnOwnerDeath(AActor* Target, ELifeStatus const Previous, ELifeStatus const New);
	FCrowdControlCallback OnRooted;
	UFUNCTION()
	void StopMotionOnRooted(FCrowdControlStatus const& Previous, FCrowdControlStatus const& New);
	FBuffEventCallback OnBuffApplied;
	UFUNCTION()
	void ApplyMoveRestrictionFromBuff(FBuffApplyEvent const& ApplyEvent);
	FBuffRemoveCallback OnBuffRemoved;
	UFUNCTION()
	void RemoveMoveRestrictionFromBuff(FBuffRemoveEvent const& RemoveEvent);

//Stats
	
public:
	
	static FGameplayTag MaxWalkSpeedStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxWalkSpeed")), false); }
	static FGameplayTag MaxCrouchSpeedStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxCrouchSpeed")), false); }
	static FGameplayTag GroundFrictionStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GroundFriction")), false); }
	static FGameplayTag BrakingDecelerationStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.BrakingDeceleration")), false); }
	static FGameplayTag MaxAccelerationStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxAcceleration")), false); }
	static FGameplayTag GravityScaleStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GravityScale")), false); }
	static FGameplayTag JumpZVelocityStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.JumpZVelocity")), false); }
	static FGameplayTag AirControlStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.AirControl")), false); }
	
private:
	
	float DefaultMaxWalkSpeed = 0.0f;
	FStatCallback MaxWalkSpeedStatCallback;
	UFUNCTION()
	void OnMaxWalkSpeedStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultCrouchSpeed = 0.0f;
	FStatCallback MaxCrouchSpeedStatCallback;
	UFUNCTION()
	void OnMaxCrouchSpeedStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultGroundFriction = 0.0f;
	FStatCallback GroundFrictionStatCallback;
	UFUNCTION()
	void OnGroundFrictionStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultBrakingDeceleration = 0.0f;
	FStatCallback BrakingDecelerationStatCallback;
	UFUNCTION()
	void OnBrakingDecelerationStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultMaxAcceleration = 0.0f;
	FStatCallback MaxAccelerationStatCallback;
	UFUNCTION()
	void OnMaxAccelerationStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultGravityScale = 0.0f;
	FStatCallback GravityScaleStatCallback;
	UFUNCTION()
	void OnGravityScaleStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultJumpZVelocity = 0.0f;
	FStatCallback JumpVelocityStatCallback;
	UFUNCTION()
	void OnJumpVelocityStatChanged(FGameplayTag const& StatTag, float const NewValue);
	float DefaultAirControl = 0.0f;
	FStatCallback AirControlStatCallback;
	UFUNCTION()
	void OnAirControlStatChanged(FGameplayTag const& StatTag, float const NewValue);

//Root Motion Sources
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf = "Source", HidePin = "Source"))
	void ApplyJumpForce(UObject* Source, ERootMotionAccumulateMode const AccumulateMode, int32 const Priority, float const Duration, FRotator const& Rotation,
		float const Distance, float const Height, bool const bFinishOnLanded, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve, bool const bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf = "Source", HidePin = "Source"))
	void ApplyConstantForce(UObject* Source, ERootMotionAccumulateMode const AccumulateMode, int32 const Priority, float const Duration, FVector const& Force,
		UCurveFloat* StrengthOverTime, bool const bIgnoreRestrictions);
	
	void RemoveRootMotionHandler(USaiyoraRootMotionHandler* Handler);
	void AddRootMotionHandlerFromReplication(USaiyoraRootMotionHandler* Handler);
	
private:
	
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> CurrentRootMotionHandlers;
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> ReplicatedRootMotionHandlers;
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> HandlersAwaitingPingDelay;
	UPROPERTY()
	TArray<UObject*> CurrentTickServerRootMotionSources;
	bool ApplyCustomRootMotionHandler(USaiyoraRootMotionHandler* Handler, UObject* Source);
	UFUNCTION()
	void DelayedHandlerApplication(USaiyoraRootMotionHandler* Handler);
};