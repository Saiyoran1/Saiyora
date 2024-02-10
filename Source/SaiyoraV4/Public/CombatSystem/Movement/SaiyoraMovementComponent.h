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

#pragma region CMC Structs

private:

	//Struct for saving off moves locally when predicting to be replayed later if needed.
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

	//Struct for managing saved moves and indicating when to apply corrections on the client.
	class FNetworkPredictionData_Client_Saiyora : public FNetworkPredictionData_Client_Character
	{
	public:
		typedef FNetworkPredictionData_Client_Character Super;
		FNetworkPredictionData_Client_Saiyora(const UCharacterMovementComponent& ClientMovement);
		virtual FSavedMovePtr AllocateNewMove() override;
	};
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	//Actual network struct sent to the server for each predicted move.
	struct FSaiyoraNetworkMoveData : public FCharacterNetworkMoveData
	{
		typedef FCharacterNetworkMoveData Super;
		FAbilityRequest CustomMoveAbilityRequest;
		int32 ServerMoveID;
		int32 ServerStatChangeID;
		virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
		virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
	};

	//Container for network move structs to send to the server.
	struct FSaiyoraNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
	{
		typedef FCharacterNetworkMoveDataContainer Super;
		FSaiyoraNetworkMoveDataContainer();
		FSaiyoraNetworkMoveData CustomDefaultMoveData[3];
	};

#pragma endregion 
#pragma region Init and Refs

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

#pragma endregion 
#pragma region Core Movement

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintPure)
	bool IsMoving() const { return bIsMoving; }
	UPROPERTY(BlueprintAssignable)
	FOnMovementChanged OnMovementChanged;

private:

	//For server-applied moves and stat changes, the server waits to apply the move until the client has received the information and confirmed that it has applied it locally.
	//This is the max time the server will wait on that confirmation before executing the move or stat change anyway.
	static constexpr float MaxMoveDelay = 0.5f;
	FSaiyoraNetworkMoveDataContainer CustomNetworkMoveDataContainer;
	//Called during ServerMove or during replaying of moves on the client. Unpacks network move data into CMC variables and then calls further move logic.
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	//Called during MoveAutonomous (on server or during replay) to unpack flags into CMC variables.
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	//Called on all machines toward the end of PerformMovement (after StartNewPhysics, which actually moves everything, and Root Motion handling, jump input, launches, etc.).
	//Is this really the right place for doing stat changes and executing custom moves? They'll be delayed one frame I think?
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	bool bIsMoving = false;

#pragma endregion 
#pragma region Custom Moves
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void TeleportToLocation(UObject* Source, const FVector& Target, const FRotator& DesiredRotation, const bool bStopMovement, const bool bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (DefaultToSelf="Source", HidePin="Source"))
	void LaunchPlayer(UObject* Source, const FVector& LaunchVector, const bool bStopMovement, const bool bIgnoreRestrictions);
	
private:

	//Called by the various Blueprint-exposed move functions to either handle prediction or alert all clients of the move.
	void ApplyCustomMove(UObject* Source, const FCustomMoveParams& CustomMove);
	//Tracks move sources already handled this tick to avoid doubling moves on listen servers (since Predicted and Server tick are called back to back).
	UPROPERTY()
	TSet<UObject*> ServerCurrentTickHandledMovement;
	//Tell all clients to execute a custom move that has been confirmed.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ExecuteCustomMove(const FCustomMoveParams& CustomMove, const bool bSkipOwner = false);
	//Called on the owning client to tell it to perform a move the server received from a non-predicted source.
	UFUNCTION(Client, Reliable)
	void Client_ExecuteServerMove(const int32 MoveID, const FCustomMoveParams& CustomMove);
	//Wraps a switch statement to actually do move logic based on the move type.
	void ExecuteCustomMove(const FCustomMoveParams& CustomMove);

	static int32 ServerMoveID;
	static int32 GenerateServerMoveID() { ServerMoveID++; return ServerMoveID; }
	//Moves the server received that it is waiting to execute until the client confirms it has executed that move locally.
	//There is a max wait time before the server just executes the move without confirmation to prevent cheating.
	TMap<int32, FServerWaitingCustomMove> WaitingServerMoves;
	//Called on the server to execute a move, either because the client confirmed it or because the wait timer expired.
	UFUNCTION()
	void ExecuteWaitingServerMove(const int32 MoveID);
	//Flag set for use in saved moves and replaying for when the server tells a client to perform a move.
	uint8 bWantsServerMove : 1;
	//ID of the move to execute, used in saved moves.
	int32 ServerMoveToExecuteID = 0;
	//Move information, used to actually perform the server move (and replay it).
	FCustomMoveParams ServerMoveToExecute;
	//Called during the movement tick when bWantsServerMove is true to execute the move.
	void ServerMoveFromFlag();
	//Flag set for use in saved moves and replaying for when the client predicts a move.
	uint8 bWantsPredictedMove : 1;
	//Client-side move struct, used for replaying the move without access to the original ability.
	FClientPendingCustomMove PendingPredictedCustomMove;
	//Ability request received by the server, used to activate an ability resulting in a custom move.
	FAbilityRequest CustomMoveAbilityRequest;
	//Map to track ability results on the predicting client, to handle mispredictions and fix replays for moves that shouldn't have happened.
	TMap<int32, bool> CompletedCastStatus;
	//Map to track movement executed on the server, to avoid doubling up on moves when receiving both an Ability RPC and movement RPC.
	TSet<FPredictedTick> ServerCompletedMovementIDs;
	//Called on the client during an ability's tick to save off information on the move.
	//Subscribes to the end of tick delegate to call OnCustomMoveCastPredicted when the tick is complete.
	void SetupCustomMovePrediction(const UCombatAbility* Source, const FCustomMoveParams& CustomMove);
	//Called when an ability tick completes so that ability parameters have been gathered for that tick.
	//This is because the movement system essentially duplicates the ability RPC as part of sending the move to the server.
	UFUNCTION()
	void OnCustomMoveCastPredicted(const FAbilityEvent& Event);
	//Called on all machines during the movement tick to perform a predicted move.
	void PredictedMoveFromFlag();
	//Delegate called when a misprediction occurs on the owning client, to undo a predicted move and prevent replays from performing the mispredicted move again.
	UFUNCTION()
	void AbilityMispredicted(const int32 PredictionID);
	
	//Flag checked during custom move calls to see if the call is from the UseAbility RPC or the custom move RPC.
	bool bUsingAbilityFromPredictedMove = false;
	//Custom moves created on the server via the AbilityComponent's UseAbility RPC, awaiting the custom move RPC to be applied.
	TMap<FPredictedTick, FCustomMoveParams> ServerConfirmedCustomMoves;
	
	void ExecuteTeleportToLocation(const FCustomMoveParams& CustomMove);
	void ExecuteLaunchPlayer(const FCustomMoveParams& CustomMove);

#pragma endregion 
#pragma region Root Motion Sources

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

#pragma endregion 
#pragma region Restrictions
	
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

#pragma endregion 
#pragma region Stats

public:

	UFUNCTION()
	float GetDefaultMaxWalkSpeed() const { return DefaultMaxWalkSpeed; }

	//Called by FastArray callbacks from PendingServerMoveStats and ConfirmedServerMoveStats.
	void UpdateMoveStatFromServer(const int32 ChangeID, const FGameplayTag StatTag, const float Value);
	
private:

	float DefaultMaxWalkSpeed = 0.0f;
	float DefaultCrouchSpeed = 0.0f;
	float DefaultGroundFriction = 0.0f;
	float DefaultBrakingDeceleration = 0.0f;
	float DefaultMaxAcceleration = 0.0f;
	float DefaultGravityScale = 0.0f;
	float DefaultJumpZVelocity = 0.0f;
	float DefaultAirControl = 0.0f;

	//Callback when any stat related to movement changes values on the server.
	UFUNCTION()
	void OnServerMoveStatChanged(const FGameplayTag StatTag, const float NewValue);
	FStatCallback StatCallback;
	//Actual execution of the stat change on the local machine.
	void UpdateMoveStat(const FGameplayTag StatTag, const float Value);
	
	//Stat changes replicated to the owning client to allow them to "predict" the stat change before the server applies it.
	UPROPERTY(Replicated)
	FServerMoveStatArray PendingServerMoveStats;
	//Stat changes replicated to non-owning clients to keep their movement stats in sync.
	UPROPERTY(Replicated)
	FServerMoveStatArray ConfirmedServerMoveStats;
	//Map to track the last update ID received from the server for each stat.
	TMap<FGameplayTag, int32> ClientLastStatUpdate;
	//Flag for saved moves to tell the server that the owning client received a stat change.
	uint8 bWantsServerStatChange : 1;
	//ID of the stat change the client is confirming.
	int32 ServerStatChangeToExecuteID = 0;
	//Actual values of the stat change sent from the server, to be used during the movement tick or for replaying on the client.
	FServerMoveStatChange ServerStatChangeToExecute;
	//Wrapper around UpdateMoveStat that is called during the movement tick to handle different net roles executing a stat change.
	void ServerStatChangeFromFlag();
	//Called by ServerStatChangeFromFlag on the server, to execute the stat change after client confirmation (or when the server's wait timer expires).
	UFUNCTION()
	void ExecuteWaitingServerStatChange(const int32 ServerStatID);

#pragma endregion 
};