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

		uint8 bSavedWantsPredictedMove : 1;
		FClientPendingCustomMove SavedPredictedCustomMove;
		uint8 bSavedPerformedServerMove : 1;
		FCustomMoveParams SavedServerMove;
		int32 SavedServerMoveID;
		uint8 bSavedPerformedServerStatChange : 1;
		FServerMoveStatChange SavedServerStatChange;
		int32 SavedServerStatChangeID;
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
		int32 ServerMoveID;
		int32 ServerStatChangeID;
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
	UFUNCTION(BlueprintPure)
	bool IsMoving() const { return bIsMoving; }
	UPROPERTY(BlueprintAssignable)
	FOnMovementChanged OnMovementChanged;

private:

	static constexpr float MaxMoveDelay = 0.5f;
	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	bool bIsMoving = false;

//Custom Moves
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportToLocation(UObject* Source, const FVector& Target, const FRotator& DesiredRotation, const bool bStopMovement, const bool bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void LaunchPlayer(UObject* Source, const FVector& LaunchVector, const bool bStopMovement, const bool bIgnoreRestrictions);
	
private:
	
	void ApplyCustomMove(UObject* Source, const FCustomMoveParams& CustomMove);
	//Tracks move sources already handled this tick to avoid doubling moves on listen servers (since Predicted and Server tick are called back to back).
	TSet<UObject*> ServerCurrentTickHandledMovement;
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMove(const FCustomMoveParams& CustomMove, const bool bSkipOwner = false);
	UFUNCTION(Client, Reliable)
	void Client_ExecuteServerMove(const int32 MoveID, const FCustomMoveParams& CustomMove);
	void ExecuteCustomMove(const FCustomMoveParams& CustomMove);

	static int32 ServerMoveID;
	static int32 GenerateServerMoveID() { ServerMoveID++; return ServerMoveID; }
	TMap<int32, FServerWaitingCustomMove> WaitingServerMoves;
	UFUNCTION()
	void ExecuteWaitingServerMove(const int32 MoveID);
	uint8 bWantsServerMove : 1;
	int32 ServerMoveToExecuteID = 0;
	FCustomMoveParams ServerMoveToExecute;
	void ServerMoveFromFlag();
	
	uint8 bWantsPredictedMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingPredictedCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	TMap<int32, bool> CompletedCastStatus;
	TSet<FPredictedTick> ServerCompletedMovementIDs;
	
	void SetupCustomMovePrediction(const UCombatAbility* Source, const FCustomMoveParams& CustomMove);
	UFUNCTION()
	void OnCustomMoveCastPredicted(const FAbilityEvent& Event);
	void PredictedMoveFromFlag();
	UFUNCTION()
	void AbilityMispredicted(const int32 PredictionID);
	
	//Flag checked during custom move calls to see if the call is from the UseAbility RPC or the custom move RPC.
	bool bUsingAbilityFromPredictedMove = false;
	//Custom moves created on the server via the AbilityComponent's UseAbility RPC, awaiting the custom move RPC to be applied.
	TMap<FPredictedTick, FCustomMoveParams> ServerConfirmedCustomMoves;
	
	void ExecuteTeleportToLocation(const FCustomMoveParams& CustomMove);
	void ExecuteLaunchPlayer(const FCustomMoveParams& CustomMove);
	
//Root Motion

public:

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf = "Source", HidePin = "Source"))
	void ApplyConstantForce(UObject* Source, const ERootMotionAccumulateMode AccumulateMode, const int32 Priority, const float Duration, const FVector& Force,
		UCurveFloat* StrengthOverTime, const bool bIgnoreRestrictions);

	void ExecuteServerRootMotion(USaiyoraRootMotionTask* Task);
	//Function called during RunGameplayTask to finally apply root motion.
	uint16 ExecuteRootMotionTask(USaiyoraRootMotionTask* Task);

private:

	void ApplyRootMotionTask(USaiyoraRootMotionTask* Task);
	UPROPERTY()
	TArray<USaiyoraRootMotionTask*> ActiveRootMotionTasks;
	UPROPERTY()
	TArray<USaiyoraRootMotionTask*> WaitingServerRootMotionTasks;
	void ExecuteWaitingServerRootMotionTask(const int32 TaskID);
	UFUNCTION(Server, Reliable)
	void Server_ConfirmClientExecutedServerRootMotion(const int32 TaskID);
	
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

public:

	UFUNCTION()
	float GetDefaultMaxWalkSpeed() const { return DefaultMaxWalkSpeed; }

	void UpdateMoveStatFromServer(const int32 ChangeID, const FGameplayTag StatTag, const float Value);
	
private:

	void UpdateMoveStat(const FGameplayTag StatTag, const float Value);
	UFUNCTION()
	void OnMoveStatChanged(const FGameplayTag StatTag, const float NewValue);
	FStatCallback StatCallback;
	
	float DefaultMaxWalkSpeed = 0.0f;
	float DefaultCrouchSpeed = 0.0f;
	float DefaultGroundFriction = 0.0f;
	float DefaultBrakingDeceleration = 0.0f;
	float DefaultMaxAcceleration = 0.0f;
	float DefaultGravityScale = 0.0f;
	float DefaultJumpZVelocity = 0.0f;
	float DefaultAirControl = 0.0f;

	UFUNCTION()
	void ExecuteWaitingServerStatChange(const int32 ServerStatID);
	UPROPERTY(Replicated)
	FServerMoveStatArray PendingServerMoveStats;
	UPROPERTY(Replicated)
	FServerMoveStatArray ConfirmedServerMoveStats;
	TMap<FGameplayTag, int32> ClientLastStatUpdate;
	uint8 bWantsServerStatChange : 1;
	int32 ServerStatChangeToExecuteID = 0;
	FServerMoveStatChange ServerStatChangeToExecute;
	void ServerStatChangeFromFlag();
};