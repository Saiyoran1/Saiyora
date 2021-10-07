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

UENUM(BlueprintType)
enum class ESaiyoraCustomMove : uint8
{
	None = 0,
	Teleport = 1,
};

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
		ESaiyoraCustomMove SavedMoveType = ESaiyoraCustomMove::None;
		FAbilityRequest SavedAbilityRequest;
		FDirectionTeleportInfo SavedDirectionTeleportInfo;
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
		ESaiyoraCustomMove CustomMoveType = ESaiyoraCustomMove::None;
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
	ESaiyoraCustomMove CustomMoveType = ESaiyoraCustomMove::None;
	FAbilityRequest CustomMoveAbilityRequest;
	FDirectionTeleportInfo DirectionTeleportInfo;
	
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
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportInDirection(UPlayerCombatAbility* Source, float const Length, bool const bShouldSweep, bool const bIgnoreZ, FCombatParameters const& PredictionParams);
private:
	void ExecuteTeleportInDirection(FDirectionTeleportInfo const& TeleportInfo);
};