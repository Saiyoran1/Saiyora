#pragma once
#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "CrowdControlStructs.h"
#include "MovementStructs.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BuffStructs.h"
#include "SaiyoraRootMotionHandler.h"
#include "StatStructs.h"
#include "SaiyoraMovementComponent.generated.h"

class UBuffHandler;
class UCrowdControlHandler;
class UDamageHandler;
class AGameState;
class USaiyoraRootMotionHandler;

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
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, const FVector& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

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
	
	USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
	UAbilityComponent* GetOwnerAbilityHandler() const { return AbilityComponentRef; }
	AGameState* GetGameStateRef() const { return GameStateRef; }
	
private:
	
	UPROPERTY()
	UAbilityComponent* AbilityComponentRef = nullptr;
	UPROPERTY()
	UCrowdControlHandler* CcHandlerRef = nullptr;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef = nullptr;
	UPROPERTY()
	UStatHandler* StatHandlerRef = nullptr;
	UPROPERTY()
	UBuffHandler* BuffHandlerRef = nullptr;
	UPROPERTY()
	AGameState* GameStateRef = nullptr;

//Core Movement

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	static const float MAXPINGDELAY;
	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;
	uint8 bWantsCustomMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	TMap<int32, bool> CompletedCastStatus;
	TSet<FPredictedTick> ServerCompletedMovementIDs;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	void SetupCustomMovementPrediction(const UCombatAbility* Source, const FCustomMoveParams& CustomMove);
	UFUNCTION()
	void OnCustomMoveCastPredicted(const FAbilityEvent& Event);
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMove(const FCustomMoveParams& CustomMove);
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMoveNoOwner(const FCustomMoveParams& CustomMove);
	UFUNCTION(Client, Unreliable)
	void Client_ExecuteCustomMove(const FCustomMoveParams& CustomMove);
	UFUNCTION()
	void DelayedCustomMoveExecution(const FCustomMoveParams CustomMove);
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	void CustomMoveFromFlag();
	void ExecuteCustomMove(const FCustomMoveParams& CustomMove);
	UFUNCTION()
	void AbilityMispredicted(const int32 PredictionID);

//Custom Moves
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportToLocation(UObject* Source, const FVector& Target, const FRotator& DesiredRotation, const bool bStopMovement, const bool bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void LaunchPlayer(UObject* Source, const FVector& LaunchVector, const bool bStopMovement, const bool bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf = "Source", HidePin = "Source"))
	void ApplyConstantForce(UObject* Source, const ERootMotionAccumulateMode AccumulateMode, const int32 Priority, const float Duration, const FVector& Force,
		UCurveFloat* StrengthOverTime, const bool bIgnoreRestrictions);

	uint16 ApplyRootMotionTask(USaiyoraRootMotionTask* Task);
	
private:
	
	void ApplyCustomMove(UObject* Source, const FCustomMoveParams& CustomMove);
	//Tracks move sources already handled this tick to avoid doubling moves on listen servers (since Predicted and Server tick are called back to back).
	TSet<UObject*> CurrentTickHandledMovement;
	//Flag checked during custom move calls to see if the call is from the UseAbility RPC or the custom move RPC.
	bool bUsingAbilityFromCustomMove = false;
	//Custom moves created on the server via the AbilityComponent's UseAbility RPC, awaiting the custom move RPC to be applied.
	TMap<FPredictedTick, FCustomMoveParams> ServerWaitingCustomMoves;
	void ExecuteTeleportToLocation(const FCustomMoveParams& CustomMove);
	void ExecuteLaunchPlayer(const FCustomMoveParams& CustomMove);

	UPROPERTY()
	TArray<USaiyoraRootMotionTask*> ActiveRootMotionTasks;
	
//Restrictions
	
private:

	UPROPERTY()
	TArray<UBuff*> MovementRestrictions;
	UPROPERTY(ReplicatedUsing = OnRep_ExternalMovementRestricted)
	bool bExternalMovementRestricted = false;
	UFUNCTION()
	void OnRep_ExternalMovementRestricted();
	virtual bool CanAttemptJump() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual FVector ConsumeInputVector() override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;
	UFUNCTION()
	void StopMotionOnOwnerDeath(AActor* Target, const ELifeStatus Previous, const ELifeStatus New);
	UFUNCTION()
	void StopMotionOnRooted(const FCrowdControlStatus& Previous, const FCrowdControlStatus& New);
	UFUNCTION()
	void ApplyMoveRestrictionFromBuff(const FBuffApplyEvent& ApplyEvent);
	UFUNCTION()
	void RemoveMoveRestrictionFromBuff(const FBuffRemoveEvent& RemoveEvent);

//Stats
	
private:
	
	float DefaultMaxWalkSpeed = 0.0f;
	FStatCallback MaxWalkSpeedStatCallback;
	UFUNCTION()
	void OnMaxWalkSpeedStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultCrouchSpeed = 0.0f;
	FStatCallback MaxCrouchSpeedStatCallback;
	UFUNCTION()
	void OnMaxCrouchSpeedStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultGroundFriction = 0.0f;
	FStatCallback GroundFrictionStatCallback;
	UFUNCTION()
	void OnGroundFrictionStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultBrakingDeceleration = 0.0f;
	FStatCallback BrakingDecelerationStatCallback;
	UFUNCTION()
	void OnBrakingDecelerationStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultMaxAcceleration = 0.0f;
	FStatCallback MaxAccelerationStatCallback;
	UFUNCTION()
	void OnMaxAccelerationStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultGravityScale = 0.0f;
	FStatCallback GravityScaleStatCallback;
	UFUNCTION()
	void OnGravityScaleStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultJumpZVelocity = 0.0f;
	FStatCallback JumpVelocityStatCallback;
	UFUNCTION()
	void OnJumpVelocityStatChanged(const FGameplayTag StatTag, const float NewValue);
	float DefaultAirControl = 0.0f;
	FStatCallback AirControlStatCallback;
	UFUNCTION()
	void OnAirControlStatChanged(const FGameplayTag StatTag, const float NewValue);
};