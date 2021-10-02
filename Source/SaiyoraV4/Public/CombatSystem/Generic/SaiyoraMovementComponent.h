// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
		FCombatParameters SavedMoveParams;
		int32 SavedMoveAbilityID = 0;
	};

	class FNetworkPredictionData_Client_Saiyora : public FNetworkPredictionData_Client_Character
	{
	public:
		typedef FNetworkPredictionData_Client_Character Super;
		FNetworkPredictionData_Client_Saiyora(const UCharacterMovementComponent& ClientMovement);
		virtual FSavedMovePtr AllocateNewMove() override;
	};

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void BeginPlay() override;

	uint8 bWantsCustomMove : 1;
	ESaiyoraCustomMove CustomMoveType = ESaiyoraCustomMove::None;
	FCombatParameters CustomMoveParams;
	int32 CustomMoveAbilityID = 0;

	UPROPERTY()
	UPlayerAbilityHandler* OwnerAbilityHandler = nullptr;
public:
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void CustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType, FCombatParameters const& Params);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void ConfirmCustomMovement(UPlayerCombatAbility* Source, ESaiyoraCustomMove const MoveType, FCombatParameters const& Params);
private:
	void ExecuteCustomMove();
};