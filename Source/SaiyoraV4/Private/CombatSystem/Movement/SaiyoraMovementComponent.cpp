#include "Movement/SaiyoraMovementComponent.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Character.h"
#include "Movement/MovementStructs.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraRootMotionHandler.h"
#include "StatHandler.h"

int32 USaiyoraMovementComponent::ServerMoveID = 0;

#pragma region Move Structs

bool USaiyoraMovementComponent::FSavedMove_Saiyora::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	if (const FSavedMove_Saiyora* CastMove = (FSavedMove_Saiyora*)&NewMove)
	{
		if (bSavedWantsPredictedMove != CastMove->bSavedWantsPredictedMove || SavedPredictedCustomMove.MoveParams.MoveType != CastMove->SavedPredictedCustomMove.MoveParams.MoveType)
		{
			return false;
		}
		if (bSavedPerformedServerMove != CastMove->bSavedPerformedServerMove || SavedServerMoveID != CastMove->SavedServerMoveID)
		{
			return false;
		}
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::Clear()
{
	Super::Clear();
	bSavedWantsPredictedMove = false;
	SavedPredictedCustomMove.Clear();
	bSavedPerformedServerMove = false;
	SavedServerMoveID = 0;
	SavedServerMove = FCustomMoveParams();
}

uint8 USaiyoraMovementComponent::FSavedMove_Saiyora::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsPredictedMove)
	{
		Result |= FLAG_Custom_1;
	}
	if (bSavedPerformedServerMove)
	{
		Result |= FLAG_Custom_2;
	}
	return Result;
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	//Used for replaying on client. Copy data from an unacked saved move into the CMC.
	if (USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement()))
	{
		Movement->PendingPredictedCustomMove = SavedPredictedCustomMove;
		Movement->ServerMoveToExecute = SavedServerMove;
	}
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::SetMoveFor(ACharacter* C, float InDeltaTime,
	const FVector& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	//Used for setting up the move before it is saved off. Sets the SavedMove params from the CMC.
	if (const USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsPredictedMove = Movement->bWantsPredictedMove;
		SavedPredictedCustomMove = Movement->PendingPredictedCustomMove;
		bSavedPerformedServerMove = Movement->bWantsServerMove;
		SavedServerMoveID = Movement->ServerMoveToExecuteID;
		SavedServerMove = Movement->ServerMoveToExecute;
	}
}

void USaiyoraMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	//Updates the CMC when replaying a client move or receiving the move on the server. Only affects flags.
	bWantsPredictedMove = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsServerMove = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
}

USaiyoraMovementComponent::FNetworkPredictionData_Client_Saiyora::FNetworkPredictionData_Client_Saiyora(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
	//I guess this just stays empty???
}

FSavedMovePtr USaiyoraMovementComponent::FNetworkPredictionData_Client_Saiyora::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Saiyora());
}

void USaiyoraMovementComponent::FSaiyoraNetworkMoveData::ClientFillNetworkMoveData(
	const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	//Sets up the Network Move Data for sending to the server or replaying. Copies Saved Move data into the Network Move Data.
	if (const FSavedMove_Saiyora* CastMove = (FSavedMove_Saiyora*)&ClientMove)
	{
		CustomMoveAbilityRequest.AbilityClass = CastMove->SavedPredictedCustomMove.AbilityClass;
		CustomMoveAbilityRequest.PredictionID = CastMove->SavedPredictedCustomMove.PredictionID;
		CustomMoveAbilityRequest.Tick = 0;
		CustomMoveAbilityRequest.Targets = CastMove->SavedPredictedCustomMove.Targets;
		CustomMoveAbilityRequest.Origin = CastMove->SavedPredictedCustomMove.Origin;
		CustomMoveAbilityRequest.ClientStartTime = CastMove->SavedPredictedCustomMove.OriginalTimestamp;
		ServerMoveID = CastMove->SavedServerMoveID;
	}
}

bool USaiyoraMovementComponent::FSaiyoraNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement,
	FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	Ar << CustomMoveAbilityRequest;
	Ar << ServerMoveID;
	return !Ar.IsError();
}

USaiyoraMovementComponent::FSaiyoraNetworkMoveDataContainer::FSaiyoraNetworkMoveDataContainer() : Super()
{
	NewMoveData = &CustomDefaultMoveData[0];
	PendingMoveData = &CustomDefaultMoveData[1];
	OldMoveData = &CustomDefaultMoveData[2];
}

FNetworkPredictionData_Client* USaiyoraMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	if (!ClientPredictionData)
	{
		USaiyoraMovementComponent* MutableThis = const_cast<USaiyoraMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Saiyora(*this);
	}
	return ClientPredictionData;
}

#pragma endregion
#pragma region Initialization

USaiyoraMovementComponent::USaiyoraMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetNetworkMoveDataContainer(CustomNetworkMoveDataContainer);
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void USaiyoraMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraMovementComponent, bExternalMovementRestricted);
}

bool USaiyoraMovementComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	if (RepFlags->bNetOwner)
	{
		bWroteSomething |= Channel->ReplicateSubobjectList(WaitingServerRootMotionTasks, *Bunch, *RepFlags);
	}
	return bWroteSomething;
}

void USaiyoraMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Custom Movement Component."));
	AbilityComponentRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
	CcHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
	MaxWalkSpeedStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnMaxWalkSpeedStatChanged);
	MaxCrouchSpeedStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnMaxCrouchSpeedStatChanged);
	GroundFrictionStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnGroundFrictionStatChanged);
	BrakingDecelerationStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnBrakingDecelerationStatChanged);
	MaxAccelerationStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnMaxAccelerationStatChanged);
	GravityScaleStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnGravityScaleStatChanged);
	JumpVelocityStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnJumpVelocityStatChanged);
	AirControlStatCallback.BindDynamic(this, &USaiyoraMovementComponent::OnAirControlStatChanged);
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	DefaultCrouchSpeed = MaxWalkSpeedCrouched;
	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDeceleration = BrakingDecelerationWalking;
	DefaultMaxAcceleration = MaxAcceleration;
	DefaultGravityScale = GravityScale;
	DefaultJumpZVelocity = JumpZVelocity;
	DefaultAirControl = AirControl;
}

void USaiyoraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GameStateRef = Cast<AGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameStateRef))
	{
		UE_LOG(LogTemp, Warning, (TEXT("Custom CMC encountered wrong Game State Ref!")));
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy && IsValid(AbilityComponentRef))
	{
		//Do not sub OnPredictedAbility, this will be added and removed only when custom moves are predicted.
		AbilityComponentRef->OnAbilityMispredicted.AddDynamic(this, &USaiyoraMovementComponent::AbilityMispredicted);
	}
	if (IsValid(DamageHandlerRef))
	{
		DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &USaiyoraMovementComponent::StopMotionOnOwnerDeath);
	}
	if (IsValid(CcHandlerRef))
	{
		CcHandlerRef->OnCrowdControlChanged.AddDynamic(this, &USaiyoraMovementComponent::StopMotionOnRooted);
	}
	if (IsValid(StatHandlerRef))
	{
		const FSaiyoraCombatTags& CombatTags = FSaiyoraCombatTags::Get();
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_MaxWalkSpeed))
		{
			MaxWalkSpeed = FMath::Max(DefaultMaxWalkSpeed * StatHandlerRef->GetStatValue(CombatTags.Stat_MaxWalkSpeed), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_MaxWalkSpeed, MaxWalkSpeedStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_MaxCrouchSpeed))
		{
			MaxWalkSpeedCrouched = FMath::Max(DefaultCrouchSpeed * StatHandlerRef->GetStatValue(CombatTags.Stat_MaxCrouchSpeed), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_MaxCrouchSpeed, MaxCrouchSpeedStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_GroundFriction))
		{
			GroundFriction = FMath::Max(DefaultGroundFriction * StatHandlerRef->GetStatValue(CombatTags.Stat_GroundFriction), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_GroundFriction, GroundFrictionStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_BrakingDeceleration))
		{
			BrakingDecelerationWalking = FMath::Max(DefaultBrakingDeceleration * StatHandlerRef->GetStatValue(CombatTags.Stat_BrakingDeceleration), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_BrakingDeceleration, BrakingDecelerationStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_MaxAcceleration))
		{
			MaxAcceleration = FMath::Max(DefaultMaxAcceleration * StatHandlerRef->GetStatValue(CombatTags.Stat_MaxAcceleration), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_MaxAcceleration, MaxAccelerationStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_GravityScale))
		{
			GravityScale = FMath::Max(DefaultGravityScale * StatHandlerRef->GetStatValue(CombatTags.Stat_GravityScale), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_GravityScale, GravityScaleStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_JumpZVelocity))
		{
			JumpZVelocity = FMath::Max(DefaultJumpZVelocity * StatHandlerRef->GetStatValue(CombatTags.Stat_JumpZVelocity), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_JumpZVelocity, JumpVelocityStatCallback);
		}
		if (StatHandlerRef->IsStatValid(CombatTags.Stat_AirControl))
		{
			AirControl = FMath::Max(DefaultAirControl * StatHandlerRef->GetStatValue(CombatTags.Stat_AirControl), 0.0f);
			StatHandlerRef->SubscribeToStatChanged(CombatTags.Stat_AirControl, AirControlStatCallback);
		}
	}
	if (IsValid(BuffHandlerRef) && GetOwnerRole() == ROLE_Authority)
	{
		BuffHandlerRef->OnIncomingBuffApplied.AddDynamic(this, &USaiyoraMovementComponent::ApplyMoveRestrictionFromBuff);
		BuffHandlerRef->OnIncomingBuffRemoved.AddDynamic(this, &USaiyoraMovementComponent::RemoveMoveRestrictionFromBuff);
	}
}

#pragma endregion
#pragma region Core Movement Functions

void USaiyoraMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	if (!CharacterOwner)
	{
		return;
	}
	if (bWantsPredictedMove)
	{
		PredictedMoveFromFlag();
		bWantsPredictedMove = false;
		CustomMoveAbilityRequest.Clear();
		PendingPredictedCustomMove.Clear();
	}
	if (bWantsServerMove)
	{
		ServerMoveFromFlag();
		bWantsServerMove = false;
		ServerMoveToExecute = FCustomMoveParams();
		ServerMoveToExecuteID = 0;
	}
}

void USaiyoraMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	const FVector& NewAccel)
{
	//Unpacks the Network Move Data for the CMC to use on the server or during replay. Copies Network Move Data into CMC.
	if (const FSaiyoraNetworkMoveData* MoveData = static_cast<FSaiyoraNetworkMoveData*>(GetCurrentNetworkMoveData()))
	{
		CustomMoveAbilityRequest = MoveData->CustomMoveAbilityRequest;
		ServerMoveToExecuteID = MoveData->ServerMoveID;
	}
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void USaiyoraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Clear this every tick, as it only exists to prevent listen servers from double-applying moves.
	//This happens because the ability system calls Predicted and Server tick of an ability back to back on listen servers.
	ServerCurrentTickHandledMovement.Empty();
	const bool bNewMove = Velocity.Length() != 0.0f;
	if (bNewMove != bIsMoving)
	{
		bIsMoving = bNewMove;
		OnMovementChanged.Broadcast(GetOwner(), bIsMoving);
	}
}

#pragma endregion
#pragma region Custom Moves

void USaiyoraMovementComponent::ApplyCustomMove(UObject* Source, const FCustomMoveParams& CustomMove)
{
	if (CustomMove.MoveType == ESaiyoraCustomMove::None)
	{
		return;
	}
	if (!IsValid(Source))
	{
		return;
	}
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return;
	}
	const UCombatAbility* SourceAsAbility = Cast<UCombatAbility>(Source);
	const bool bExternal = !IsValid(SourceAsAbility) || SourceAsAbility->GetHandler()->GetOwner() != GetOwner();
	//If not ignoring restrictions, check for roots or for active movement restriction.
	if (!CustomMove.bIgnoreRestrictions)
	{
		if (IsValid(CcHandlerRef) && CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return;
		}
		if (bExternal && bExternalMovementRestricted)
		{
			return;
		}
	}
	if (bExternal || SourceAsAbility->GetPredictionID() == 0)
	{
		//Not dealing with prediction, must happen on server.
		if (GetOwnerRole() != ROLE_Authority)
		{
			return;
		}
		//Avoid doubling up in the case of listen servers moving on Predicted and Server tick back to back. This also places a limit of one custom move per source per game tick.
		if (ServerCurrentTickHandledMovement.Contains(Source))
		{
			return;
		}
		ServerCurrentTickHandledMovement.Add(Source);
		if (PawnOwner->IsLocallyControlled())
		{
			Multicast_ExecuteCustomMove(CustomMove);
		}
		else
		{
			//Generate an ID for this move, and then wait until the client sends a move that says it has executed this move ID to then execute it on server.
			const int32 WaitingMoveID = GenerateServerMoveID();
			Client_ExecuteServerMove(WaitingMoveID, CustomMove);
			FServerWaitingCustomMove WaitingMove = FServerWaitingCustomMove(CustomMove);
			//Set a timer for a max amount of time we will wait, if the client doesn't send by this point, execute the move anyway.
			const FTimerDelegate WaitingMoveDelegate = FTimerDelegate::CreateUObject(this, &USaiyoraMovementComponent::ExecuteWaitingServerMove, WaitingMoveID);
			GetWorld()->GetTimerManager().SetTimer(WaitingMove.TimerHandle, WaitingMoveDelegate, MaxMoveDelay, false);
			WaitingServerMoves.Add(WaitingMoveID, WaitingMove);
		}
	}
	else
	{
		//Dealing with prediction.
		switch (GetOwnerRole())
		{
		case ROLE_Authority :
			{
				if (bUsingAbilityFromPredictedMove)
				{
					//We received the RPC from the client that requested an ability use, and the ability is now requesting custom movement.
					Multicast_ExecuteCustomMove(CustomMove, true);
				}
				else
				{
					//We didn't yet receive a client RPC requesting this move, the ability RPC arrived first, so we can just save that the ability was successful.
					//When we get a move RPC that requests this predicted tick, we don't re-perform the ability, we just perform the move.
					ServerCompletedMovementIDs.Add(FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick()));
					ServerConfirmedCustomMoves.Add(FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick()), CustomMove);
				}
			}
			break;
		case ROLE_AutonomousProxy :
			{
				//GAS calls this before ability usage to avoid some net corrections.
				FlushServerMoves();
				SetupCustomMovePrediction(SourceAsAbility, CustomMove);
			}
			break;
		default :
			return;
		}
	}
}

void USaiyoraMovementComponent::ExecuteCustomMove(const FCustomMoveParams& CustomMove)
{
	switch (CustomMove.MoveType)
	{
	case ESaiyoraCustomMove::Launch :
		ExecuteLaunchPlayer(CustomMove);
		break;
	case ESaiyoraCustomMove::Teleport :
		ExecuteTeleportToLocation(CustomMove);
		break;
	default:
		break;
	}
}

void USaiyoraMovementComponent::Client_ExecuteServerMove_Implementation(const int32 MoveID, const FCustomMoveParams& CustomMove)
{
	bWantsServerMove = true;
	ServerMoveToExecute = CustomMove;
	ServerMoveToExecuteID = MoveID;
}

void USaiyoraMovementComponent::Multicast_ExecuteCustomMove_Implementation(const FCustomMoveParams& CustomMove, const bool bSkipOwner)
{
	if (bSkipOwner && GetOwnerRole() == ROLE_AutonomousProxy)
	{
		return;
	}
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::ExecuteWaitingServerMove(const int32 MoveID)
{
	FServerWaitingCustomMove* WaitingMove = WaitingServerMoves.Find(MoveID);
	if (!WaitingMove)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(WaitingMove->TimerHandle);
	Multicast_ExecuteCustomMove(WaitingMove->MoveParams, true);
	WaitingServerMoves.Remove(MoveID);
}

void USaiyoraMovementComponent::ServerMoveFromFlag()
{
	if (GetOwnerRole() == ROLE_Authority && PawnOwner->IsLocallyControlled())
	{
		//Bypass need for flags on listen servers.
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
	{
		ExecuteWaitingServerMove(ServerMoveToExecuteID);
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		ExecuteCustomMove(ServerMoveToExecute);
	}
}

void USaiyoraMovementComponent::SetupCustomMovePrediction(const UCombatAbility* Source, const FCustomMoveParams& CustomMove)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || CustomMove.MoveType == ESaiyoraCustomMove::None || !IsValid(Source) || !IsValid(AbilityComponentRef))
	{
		return;
	}
	AbilityComponentRef->OnAbilityTick.AddDynamic(this, &USaiyoraMovementComponent::OnCustomMoveCastPredicted);
	PendingPredictedCustomMove.AbilityClass = Source->GetClass();
	PendingPredictedCustomMove.MoveParams = CustomMove;
	PendingPredictedCustomMove.PredictionID = Source->GetPredictionID();
	PendingPredictedCustomMove.OriginalTimestamp = GameStateRef->GetServerWorldTimeSeconds();
}

void USaiyoraMovementComponent::OnCustomMoveCastPredicted(const FAbilityEvent& Event)
{
	AbilityComponentRef->OnAbilityTick.RemoveDynamic(this, &USaiyoraMovementComponent::OnCustomMoveCastPredicted);
	CompletedCastStatus.Add(Event.PredictionID, true);
	if (!IsValid(PendingPredictedCustomMove.AbilityClass) || PendingPredictedCustomMove.AbilityClass != Event.Ability->GetClass()
		|| PendingPredictedCustomMove.PredictionID != Event.PredictionID || PendingPredictedCustomMove.MoveParams.MoveType == ESaiyoraCustomMove::None)
	{
		PendingPredictedCustomMove.Clear();
		return;
	}
	PendingPredictedCustomMove.Targets = Event.Targets;
	PendingPredictedCustomMove.Origin = Event.Origin;
	bWantsPredictedMove = true;
}

void USaiyoraMovementComponent::PredictedMoveFromFlag()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && PawnOwner->IsLocallyControlled())
	{
		//If we are a listen server, bypass the need for flags. The movement itself is just manually called by ability.
		return;
	}
	if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
	{
		//If we are the server, and have an auto proxy, check that this move hasn't already been sent (duplicating cast IDs), and can actually be performed.
		if (ServerCompletedMovementIDs.Contains(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick)) || !IsValid(AbilityComponentRef))
		{
			FCustomMoveParams* ExistingMove = ServerConfirmedCustomMoves.Find(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			if (ExistingMove)
			{
				Multicast_ExecuteCustomMove(*ExistingMove, true);
				ServerConfirmedCustomMoves.Remove(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			}
		}
		else
		{
			bUsingAbilityFromPredictedMove = true;
			if (AbilityComponentRef->UseAbilityFromPredictedMovement(CustomMoveAbilityRequest))
			{
				//Document that this prediction ID has already been used, so duplicate moves with this ID do not get re-used.
				ServerCompletedMovementIDs.Add(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			}
			bUsingAbilityFromPredictedMove = false;
		}
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//Check to see if the pending move has been marked as a misprediction, to prevent it from being replayed.
		if (!CompletedCastStatus.FindRef(PendingPredictedCustomMove.PredictionID))
		{
			return;
		}
		ExecuteCustomMove(PendingPredictedCustomMove.MoveParams);
	}
}

void USaiyoraMovementComponent::AbilityMispredicted(const int32 PredictionID)
{
	CompletedCastStatus.Add(PredictionID, false);
}

void USaiyoraMovementComponent::TeleportToLocation(UObject* Source, const FVector& Target, const FRotator& DesiredRotation, const bool bStopMovement, const bool bIgnoreRestrictions)
{
	FCustomMoveParams TeleParams;
	TeleParams.MoveType = ESaiyoraCustomMove::Teleport;
	TeleParams.Target = Target;
	TeleParams.Rotation = DesiredRotation;
	TeleParams.bStopMovement = bStopMovement;
	TeleParams.bIgnoreRestrictions = bIgnoreRestrictions;
	ApplyCustomMove(Source, TeleParams);
}

void USaiyoraMovementComponent::ExecuteTeleportToLocation(const FCustomMoveParams& CustomMove)
{
	GetOwner()->SetActorLocation(CustomMove.Target);
	GetOwner()->SetActorRotation(CustomMove.Rotation);
	if (CustomMove.bStopMovement)
	{
		StopMovementImmediately();
	}
}

void USaiyoraMovementComponent::LaunchPlayer(UObject* Source, const FVector& LaunchVector, const bool bStopMovement, const bool bIgnoreRestrictions)
{
	FCustomMoveParams LaunchParams;
	LaunchParams.MoveType = ESaiyoraCustomMove::Launch;
	LaunchParams.Target = LaunchVector;
	LaunchParams.bStopMovement = bStopMovement;
	LaunchParams.bIgnoreRestrictions = bIgnoreRestrictions;
	ApplyCustomMove(Source, LaunchParams);
}

void USaiyoraMovementComponent::ExecuteLaunchPlayer(const FCustomMoveParams& CustomMove)
{
	if (CustomMove.bStopMovement)
	{
		StopMovementImmediately();
	}
	Launch(CustomMove.Target);
}

#pragma endregion
#pragma region Root Motion

void USaiyoraMovementComponent::ApplyConstantForce(UObject* Source, const ERootMotionAccumulateMode AccumulateMode, const int32 Priority,
	const float Duration, const FVector& Force, UCurveFloat* StrengthOverTime, const bool bIgnoreRestrictions)
{
	USaiyoraConstantForce* ConstantForce = UGameplayTask::NewTask<USaiyoraConstantForce>(AbilityComponentRef);
	ConstantForce->Init(this, Source);
	ConstantForce->Duration = Duration;
	ConstantForce->Priority = Priority;
	ConstantForce->AccumulateMode = AccumulateMode;
	ConstantForce->Force = Force;
	ConstantForce->StrengthOverTime = StrengthOverTime;
	ConstantForce->bIgnoreRestrictions = bIgnoreRestrictions;
	ApplyRootMotionTask(ConstantForce);
}

void USaiyoraMovementComponent::ApplyRootMotionTask(USaiyoraRootMotionTask* Task)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	if (!IsValid(Task))
	{
		return;
	}
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return;
	}
	//If not ignoring restrictions, check for roots or for active movement restriction.
	if (!Task->bIgnoreRestrictions)
	{
		if (IsValid(CcHandlerRef) && CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return;
		}
		if (Task->IsExternal() && bExternalMovementRestricted)
		{
			return;
		}
	}
	if (!PawnOwner->IsLocallyControlled() && (Task->IsExternal() || Task->GetPredictedTick().PredictionID == 0))
	{
		//Generate an ID for this move, and then wait until the client sends an RPC that says it has executed this move ID to then execute it on server.
		Task->ServerWaitID = GenerateServerMoveID();
		//Set a timer for a max amount of time we will wait, if the client doesn't send by this point, execute the move anyway.
		const FTimerDelegate WaitingMoveDelegate = FTimerDelegate::CreateUObject(this, &USaiyoraMovementComponent::ExecuteWaitingServerRootMotionTask, Task->ServerWaitID);
		GetWorld()->GetTimerManager().SetTimer(Task->ServerWaitHandle, WaitingMoveDelegate, MaxMoveDelay, false);
		WaitingServerRootMotionTasks.Add(Task);
		ForceReplicationUpdate();
	}
	else
	{
		if (IsValid(AbilityComponentRef))
		{
			AbilityComponentRef->RunGameplayTask(*AbilityComponentRef, *Task, 0, FGameplayResourceSet(), FGameplayResourceSet());
		}
	}
}

void USaiyoraMovementComponent::ExecuteWaitingServerRootMotionTask(const int32 TaskID)
{
	for (USaiyoraRootMotionTask* WaitingTask : WaitingServerRootMotionTasks)
	{
		if (WaitingTask->ServerWaitID == TaskID)
		{
			WaitingServerRootMotionTasks.Remove(WaitingTask);
			GetWorld()->GetTimerManager().ClearTimer(WaitingTask->ServerWaitHandle);
			if (IsValid(AbilityComponentRef))
			{
				AbilityComponentRef->RunGameplayTask(*AbilityComponentRef, *WaitingTask, 0, FGameplayResourceSet(), FGameplayResourceSet());
			}
			//TODO: How to run root motion with no ability comp?
			return;
		}
	}
}

void USaiyoraMovementComponent::Server_ConfirmClientExecutedServerRootMotion_Implementation(const int32 TaskID)
{
	ExecuteWaitingServerRootMotionTask(TaskID);
}

uint16 USaiyoraMovementComponent::ExecuteRootMotionTask(USaiyoraRootMotionTask* Task)
{
	if (!IsValid(Task))
	{
		return (uint16)ERootMotionSourceID::Invalid;
	}
	if (!Task->bIgnoreRestrictions && IsValid(CcHandlerRef) && CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
	{
		return (uint16)ERootMotionSourceID::Invalid;		
	}
	if (!Task->bIgnoreRestrictions && Task->IsExternal() && bExternalMovementRestricted)
	{
		return (uint16)ERootMotionSourceID::Invalid;
	}
	ActiveRootMotionTasks.Add(Task);
	return ApplyRootMotionSource(Task->GetRootMotionSource());
}

void USaiyoraMovementComponent::ExecuteServerRootMotion(USaiyoraRootMotionTask* Task)
{
	if (PawnOwner->IsLocallyControlled())
	{
		if (IsValid(AbilityComponentRef))
		{
			AbilityComponentRef->RunGameplayTask(*AbilityComponentRef, *Task, 0, FGameplayResourceSet(), FGameplayResourceSet());
		}
		Server_ConfirmClientExecutedServerRootMotion(Task->ServerWaitID);
	}
}

#pragma endregion
#pragma region Restrictions

void USaiyoraMovementComponent::ApplyMoveRestrictionFromBuff(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(ApplyEvent.AffectedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	ApplyEvent.AffectedBuff->GetBuffTags(BuffTags);
	if (BuffTags.HasTagExact(FSaiyoraCombatTags::Get().MovementRestriction))
	{
		const bool bPreviouslyRestricted = bExternalMovementRestricted;
		MovementRestrictions.AddUnique(ApplyEvent.AffectedBuff);
		if (!bPreviouslyRestricted && MovementRestrictions.Num() > 0)
		{
			bExternalMovementRestricted = true;
			for (USaiyoraRootMotionTask* ActiveRMTask : ActiveRootMotionTasks)
			{
				if (!ActiveRMTask->bIgnoreRestrictions && ActiveRMTask->IsExternal())
				{
					ActiveRMTask->EndTask();
				}
			}
		}
	}
}

void USaiyoraMovementComponent::RemoveMoveRestrictionFromBuff(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	RemoveEvent.RemovedBuff->GetBuffTags(BuffTags);
	if (BuffTags.HasTagExact(FSaiyoraCombatTags::Get().MovementRestriction))
	{
		const bool bPreviouslyRestricted = bExternalMovementRestricted;
		MovementRestrictions.Remove(RemoveEvent.RemovedBuff);
		if (bPreviouslyRestricted && MovementRestrictions.Num() == 0)
		{
			bExternalMovementRestricted = false;
		}
	}
}

void USaiyoraMovementComponent::OnRep_ExternalMovementRestricted()
{
	if (bExternalMovementRestricted)
	{
		for (USaiyoraRootMotionTask* ActiveRMTask : ActiveRootMotionTasks)
		{
			if (!ActiveRMTask->bIgnoreRestrictions && ActiveRMTask->IsExternal())
			{
				ActiveRMTask->EndTask();
			}
		}
	}
}

void USaiyoraMovementComponent::StopMotionOnOwnerDeath(AActor* Target, const ELifeStatus Previous,
	const ELifeStatus New)
{
	if (Target == GetOwner() && New != ELifeStatus::Alive)
	{
		for (USaiyoraRootMotionTask* ActiveRMTask : ActiveRootMotionTasks)
		{
			ActiveRMTask->EndTask();
		}
		StopMovementImmediately();
	}
}

void USaiyoraMovementComponent::StopMotionOnRooted(const FCrowdControlStatus& Previous, const FCrowdControlStatus& New)
{
	//TODO: Can add ping delay to root taking effect for auto proxies? How does this work with sim proxies?
	if (New.CrowdControlType == FSaiyoraCombatTags::Get().Cc_Root && New.bActive)
	{
		for (USaiyoraRootMotionTask* ActiveRMTask : ActiveRootMotionTasks)
		{
			if (IsValid(ActiveRMTask) && !ActiveRMTask->bIgnoreRestrictions)
			{
				ActiveRMTask->EndTask();
			}
		}
		StopMovementImmediately();
	}
}

#pragma endregion
#pragma region Getter Overrides

bool USaiyoraMovementComponent::CanAttemptJump() const
{
	if (!Super::CanAttemptJump())
	{
		return false;
	}
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return false;
	}
	if (IsValid(CcHandlerRef))
	{
		if (CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Stun) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Incapacitate) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return false;
		}
	}
	return true;
}

bool USaiyoraMovementComponent::CanCrouchInCurrentState() const
{
	if (!Super::CanCrouchInCurrentState())
	{
		return false;
	}
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return false;
	}
	if (IsValid(CcHandlerRef))
	{
		if (CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Stun) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Incapacitate))
		{
			return false;
		}
	}
	return true;
}

FVector USaiyoraMovementComponent::ConsumeInputVector()
{
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return FVector::ZeroVector;
	}
	if (IsValid(CcHandlerRef))
	{
		if (CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Stun) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Incapacitate) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return FVector::ZeroVector;
		}
	}
	return Super::ConsumeInputVector();
}

float USaiyoraMovementComponent::GetMaxAcceleration() const
{
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return 0.0f;
	}
	if (IsValid(CcHandlerRef))
	{
		if (CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Stun) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Incapacitate) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return 0.0f;
		}
	}
	return Super::GetMaxAcceleration();
}

float USaiyoraMovementComponent::GetMaxSpeed() const
{
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return 0.0f;
	}
	if (IsValid(CcHandlerRef))
	{
		if (CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Stun) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Incapacitate) || CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return 0.0f;
		}
	}
	return Super::GetMaxSpeed();
}

#pragma endregion
#pragma region Stats

void USaiyoraMovementComponent::OnMaxWalkSpeedStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	//TODO: Delay by ping for non-locally controlled on the server?
	//Make sure all movement stats are actually replicated.
	//Need a way for remote clients to see these changes?
	MaxWalkSpeed = FMath::Max(DefaultMaxWalkSpeed * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnMaxCrouchSpeedStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	MaxWalkSpeedCrouched = FMath::Max(DefaultCrouchSpeed * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnGroundFrictionStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	GroundFriction = FMath::Max(DefaultGroundFriction * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnBrakingDecelerationStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	BrakingDecelerationWalking = FMath::Max(DefaultBrakingDeceleration * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnMaxAccelerationStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	MaxAcceleration = FMath::Max(DefaultMaxAcceleration * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnGravityScaleStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	GravityScale = FMath::Max(DefaultGravityScale * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnJumpVelocityStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	JumpZVelocity = FMath::Max(DefaultJumpZVelocity * NewValue, 0.0f);
}

void USaiyoraMovementComponent::OnAirControlStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	AirControl = FMath::Max(DefaultAirControl * NewValue, 0.0f);
}

#pragma endregion