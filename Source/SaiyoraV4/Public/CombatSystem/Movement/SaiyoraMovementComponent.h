// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "MovementEnums.h"
#include "MovementStructs.h"
#include "PlayerAbilityHandler.h"
#include "PlayerCombatAbility.h"
#include "SaiyoraRootMotionHandler.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SaiyoraMovementComponent.generated.h"

UCLASS(BlueprintType)
class SAIYORAV4_API USaiyoraMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
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

	static const float MaxPingDelay;
	
public:
	USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer);
	UPlayerAbilityHandler* GetOwnerAbilityHandler() const { return OwnerAbilityHandler; }
	ASaiyoraGameState* GetGameStateRef() const { return GameStateRef; }
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void BeginPlay() override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;
	UPROPERTY()
	UPlayerAbilityHandler* OwnerAbilityHandler = nullptr;
	UPROPERTY()
	class UCrowdControlHandler* OwnerCcHandler = nullptr;
	UPROPERTY()
	UDamageHandler* OwnerDamageHandler = nullptr;
	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	FLifeStatusCallback OnDeath;
	UFUNCTION()
	void StopMotionOnOwnerDeath(AActor* Target, ELifeStatus const Previous, ELifeStatus const New);
	FCrowdControlCallback OnRooted;
	UFUNCTION()
	void StopMotionOnRooted(FCrowdControlStatus const& Previous, FCrowdControlStatus const& New);

	//Custom Movement

	bool ApplyCustomMove(FCustomMoveParams const& CustomMove, UObject* Source);
	void ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	UFUNCTION()
	void DelayedCustomMoveExecution(FCustomMoveParams CustomMove);
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMoveNoOwner(FCustomMoveParams const& CustomMove);
	UFUNCTION(Client, Unreliable)
	void Client_ExecuteCustomMove(FCustomMoveParams const& CustomMove);
	void SetupCustomMovementPrediction(UPlayerCombatAbility* Source, FCustomMoveParams const& CustomMove);
	FAbilityCallback OnPredictedAbility;
	UFUNCTION()
	void OnCustomMoveCastPredicted(FCastEvent const& Event);
	void CustomMoveFromFlag();
	FAbilityMispredictionCallback OnMispredict;
	UFUNCTION()
	void AbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason);
	TMap<int32, bool> CompletedCastStatus;
	TSet<int32> ServerCompletedMovementIDs;
	TArray<UObject*> CurrentTickServerCustomMoveSources;
	uint8 bWantsCustomMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	bool TeleportToLocation(UObject* Source, FVector const& Target, FRotator const& DesiredRotation, bool const bStopMovement, bool const bIgnoreRestrictions);
private:
	void ExecuteTeleportToLocation(FCustomMoveParams const& CustomMove);
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	bool LaunchPlayer(UPlayerCombatAbility* Source, FVector const& LaunchVector, bool const bStopMovement, bool const bIgnoreRestrictions);
private:
	void ExecuteLaunchPlayer(FCustomMoveParams const& CustomMove);

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
	UFUNCTION()
	void DelayedHandlerApplication(USaiyoraRootMotionHandler* Handler);
	UPROPERTY()
	TArray<UObject*> CurrentTickServerRootMotionSources;
	bool ApplyCustomRootMotionHandler(USaiyoraRootMotionHandler* Handler, UObject* Source);
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> CurrentRootMotionHandlers;
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> ReplicatedRootMotionHandlers;
	UPROPERTY()
	TArray<USaiyoraRootMotionHandler*> HandlersAwaitingPingDelay;
	
	//Restrictions
public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement")
	void AddExternalMovementRestriction(FExternalMovementCondition const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement")
	void RemoveExternalMovementRestriction(FExternalMovementCondition const& Restriction);
private:
	bool CheckExternalMoveRestricted(UObject* Source, ESaiyoraCustomMove const MoveType);
	TArray<FExternalMovementCondition> MovementRestrictions;
	virtual bool CanAttemptJump() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual FVector ConsumeInputVector() override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;
};