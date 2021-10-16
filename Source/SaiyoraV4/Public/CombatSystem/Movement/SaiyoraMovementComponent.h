// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovementEnums.h"
#include "MovementStructs.h"
#include "PlayerAbilityHandler.h"
#include "PlayerCombatAbility.h"
#include "SaiyoraStructs.h"
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
	
public:
	USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer);
private:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void BeginPlay() override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;

	uint8 bWantsCustomMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	
	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;

	UPROPERTY()
	UPlayerAbilityHandler* OwnerAbilityHandler = nullptr;
	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;

	void ExecuteCustomMove();
	FAbilityMispredictionCallback OnMispredict;
	UFUNCTION()
	void AbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason);
	TMap<int32, bool> CompletedCastStatus;
	TSet<int32> ServerCompletedMovementIDs;

	//Movement Functions
private:
	void SetupCustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType, FCustomMoveParams const& Params);
	FAbilityCallback OnPredictedAbility;
	UFUNCTION()
	void OnCustomMoveCastPredicted(FCastEvent const& Event);
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void PredictTeleportInDirection(UPlayerCombatAbility* Source, FVector const& Direction, float const Length, bool const bSweep, bool const bIgnoreZ);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportInDirection(UPlayerCombatAbility* Source, FVector const& Direction, float const Length, bool const bSweep, bool const bIgnoreZ);
private:
	void ExecuteTeleportInDirection(FVector const& Direction, float const Length, bool const bSweep, bool const bIgnoreZ);
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void PredictTeleportToLocation(UPlayerCombatAbility* Source, FVector const& Target, FRotator const& DesiredRotation, bool const bSweep);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportToLocation(UPlayerCombatAbility* Source, FVector const& Target, FRotator const& DesiredRotation, bool const bSweep);
private:
	void ExecuteTeleportToLocation(FVector const& Target, FRotator const& DesiredRotation, bool const bSweep);
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void PredictLaunchPlayer(UPlayerCombatAbility* Source, FVector const& Direction, float const Force);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void LaunchPlayer(UPlayerCombatAbility* Source, FVector const& Direction, float const Force);
private:
	void ExecuteLaunchPlayer(FVector const& Direction, float const Force);

	//Root Motion Sources?
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TestRootMotion(UPlayerCombatAbility* Source);
private:
	UFUNCTION()
	void CleanupRootMotion(uint16 const ID);
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void JumpForce(UPlayerCombatAbility* Source, FRotator Rotation, float Distance, float Height, float Duration, float MinimumLandedTriggerTime,
	bool bFinishOnLanded, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve);
};

USTRUCT()
struct FCustomRootMotionSource : public FRootMotionSource
{
	GENERATED_BODY()

	FCustomRootMotionSource();
	virtual FRootMotionSource* Clone() const override;
	virtual bool Matches(const FRootMotionSource* Other) const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override;

	UPROPERTY()
	int32 PredictionID = 0;
};

template<>
struct TStructOpsTypeTraits<FCustomRootMotionSource> : public TStructOpsTypeTraitsBase2<FCustomRootMotionSource>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

USTRUCT()
struct FCustomJumpForce : public FCustomRootMotionSource
{
	GENERATED_USTRUCT_BODY()

	FCustomJumpForce();
	virtual ~FCustomJumpForce() {}

	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	float Distance;
	UPROPERTY()
	float Height;
	UPROPERTY()
	bool bDisableTimeout;
	UPROPERTY()
	UCurveVector* PathOffsetCurve;
	UPROPERTY()
	UCurveFloat* TimeMappingCurve;
	FVector SavedHalfwayLocation;

	FVector GetPathOffset(float MoveFraction) const;
	FVector GetRelativeLocation(float MoveFraction) const;
	virtual bool IsTimeOutEnabled() const override;
	virtual FRootMotionSource* Clone() const override;
	virtual bool Matches(const FRootMotionSource* Other) const override;
	virtual void PrepareRootMotion(
		float SimulationTime, 
		float MovementTickTime,
		const ACharacter& Character, 
		const UCharacterMovementComponent& MoveComponent
		) override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FString ToSimpleString() const override;
	virtual void AddReferencedObjects(class FReferenceCollector& Collector) override;
};
