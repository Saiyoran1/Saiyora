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

float const USaiyoraMovementComponent::MAXPINGDELAY = 0.2f;

#pragma region Move Structs

bool USaiyoraMovementComponent::FSavedMove_Saiyora::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	if (const FSavedMove_Saiyora* CastMove = (FSavedMove_Saiyora*)&NewMove)
	{
		if (bSavedWantsCustomMove != CastMove->bSavedWantsCustomMove || SavedPendingCustomMove.MoveParams.MoveType != CastMove->SavedPendingCustomMove.MoveParams.MoveType)
		{
			return false;
		}
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::Clear()
{
	Super::Clear();
	bSavedWantsCustomMove = false;
	SavedPendingCustomMove.Clear();
}

uint8 USaiyoraMovementComponent::FSavedMove_Saiyora::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsCustomMove)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	//Used for replaying on client. Copy data from an unacked saved move into the CMC.
	if (USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement()))
	{
		Movement->PendingCustomMove = SavedPendingCustomMove;
	}
}

void USaiyoraMovementComponent::FSavedMove_Saiyora::SetMoveFor(ACharacter* C, float InDeltaTime,
	const FVector& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	//Used for setting up the move before it is saved off. Sets the SavedMove params from the CMC.
	if (const USaiyoraMovementComponent* Movement = Cast<USaiyoraMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsCustomMove = Movement->bWantsCustomMove;
		SavedPendingCustomMove = Movement->PendingCustomMove;
	}
}

void USaiyoraMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	//Updates the CMC when replaying a client move or receiving the move on the server. Only affects flags.
	bWantsCustomMove = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
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
		CustomMoveAbilityRequest.AbilityClass = CastMove->SavedPendingCustomMove.AbilityClass;
		CustomMoveAbilityRequest.PredictionID = CastMove->SavedPendingCustomMove.PredictionID;
		CustomMoveAbilityRequest.Tick = 0;
		CustomMoveAbilityRequest.Targets = CastMove->SavedPendingCustomMove.Targets;
		CustomMoveAbilityRequest.Origin = CastMove->SavedPendingCustomMove.Origin;
		CustomMoveAbilityRequest.ClientStartTime = CastMove->SavedPendingCustomMove.OriginalTimestamp;
	}
}

bool USaiyoraMovementComponent::FSaiyoraNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement,
	FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	Ar << CustomMoveAbilityRequest;
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

bool USaiyoraMovementComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	if (RepFlags->bNetOwner)
	{
		if (HandlersAwaitingPingDelay.Num() > 0)
		{
			bWroteSomething |= Channel->ReplicateSubobjectList(HandlersAwaitingPingDelay, *Bunch, *RepFlags);
		}
		if (ReplicatedRootMotionHandlers.Num() > 0)
		{
			for (USaiyoraRootMotionHandler* Handler : ReplicatedRootMotionHandlers)
			{
				if (IsValid(Handler) && Handler->GetPredictionID() == 0)
				{
					bWroteSomething |= Channel->ReplicateSubobject(Handler, *Bunch, *RepFlags);
				}
			}
		}
	}
	else
	{
		if (ReplicatedRootMotionHandlers.Num() > 0)
		{
			bWroteSomething |= Channel->ReplicateSubobjectList(ReplicatedRootMotionHandlers, *Bunch, *RepFlags);
		}
	}
	return bWroteSomething;
}

void USaiyoraMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USaiyoraMovementComponent, bExternalMovementRestricted);
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
	if (bWantsCustomMove)
	{
		CustomMoveFromFlag();
		bWantsCustomMove = false;
		CustomMoveAbilityRequest.Clear();
		PendingCustomMove.Clear();
	}
}

void USaiyoraMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	const FVector& NewAccel)
{
	//Unpacks the Network Move Data for the CMC to use on the server or during replay. Copies Network Move Data into CMC.
	if (const FSaiyoraNetworkMoveData* MoveData = static_cast<FSaiyoraNetworkMoveData*>(GetCurrentNetworkMoveData()))
	{
		CustomMoveAbilityRequest = MoveData->CustomMoveAbilityRequest;
	}
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void USaiyoraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Clear this every tick, as it only exists to prevent listen servers from double-applying moves.
	//This happens because the ability system calls Predicted and Server tick of an ability back to back on listen servers.
	CurrentTickHandledMovement.Empty();
}

#pragma endregion
#pragma region Custom Moves

void USaiyoraMovementComponent::ExecuteCustomMove(const FCustomMoveParams& CustomMove)
{
	//If not ignoring restrictions, check for roots or for active movement restriction.
	if (!CustomMove.bIgnoreRestrictions)
	{
		if (IsValid(CcHandlerRef) && CcHandlerRef->IsCrowdControlActive(FSaiyoraCombatTags::Get().Cc_Root))
		{
			return;
		}
		if (CustomMove.bExternal && bExternalMovementRestricted)
		{
			return;
		}
	}
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

void USaiyoraMovementComponent::DelayedCustomMoveExecution(const FCustomMoveParams CustomMove)
{
	Multicast_ExecuteCustomMoveNoOwner(CustomMove);
}

void USaiyoraMovementComponent::Multicast_ExecuteCustomMove_Implementation(const FCustomMoveParams& CustomMove)
{
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::Multicast_ExecuteCustomMoveNoOwner_Implementation(const FCustomMoveParams& CustomMove)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		return;
	}
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::Client_ExecuteCustomMove_Implementation(const FCustomMoveParams& CustomMove)
{
	ExecuteCustomMove(CustomMove);
}

void USaiyoraMovementComponent::SetupCustomMovementPrediction(const UCombatAbility* Source, const FCustomMoveParams& CustomMove)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || CustomMove.MoveType == ESaiyoraCustomMove::None || !IsValid(Source) || !IsValid(AbilityComponentRef))
	{
		return;
	}
	AbilityComponentRef->OnAbilityTick.AddDynamic(this, &USaiyoraMovementComponent::OnCustomMoveCastPredicted);
	PendingCustomMove.AbilityClass = Source->GetClass();
	PendingCustomMove.MoveParams = CustomMove;
	PendingCustomMove.PredictionID = Source->GetPredictionID();
	PendingCustomMove.OriginalTimestamp = GameStateRef->GetServerWorldTimeSeconds();
}

void USaiyoraMovementComponent::OnCustomMoveCastPredicted(const FAbilityEvent& Event)
{
	AbilityComponentRef->OnAbilityTick.RemoveDynamic(this, &USaiyoraMovementComponent::OnCustomMoveCastPredicted);
	CompletedCastStatus.Add(Event.PredictionID, true);
	if (!IsValid(PendingCustomMove.AbilityClass) || PendingCustomMove.AbilityClass != Event.Ability->GetClass()
		|| PendingCustomMove.PredictionID != Event.PredictionID || PendingCustomMove.MoveParams.MoveType == ESaiyoraCustomMove::None)
	{
		PendingCustomMove.Clear();
		return;
	}
	PendingCustomMove.Targets = Event.Targets;
	PendingCustomMove.Origin = Event.Origin;
	bWantsCustomMove = true;
}

void USaiyoraMovementComponent::CustomMoveFromFlag()
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
			FCustomMoveParams* ExistingMove = ServerWaitingCustomMoves.Find(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			if (ExistingMove)
			{
				Multicast_ExecuteCustomMoveNoOwner(*ExistingMove);
				ServerWaitingCustomMoves.Remove(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			}
			else
			{
				USaiyoraRootMotionHandler* ExistingHandler = ServerWaitingRootMotion.FindRef(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
				if (IsValid(ExistingHandler))
				{
					//TODO: Root checks?
					ExistingHandler->Init(this, CustomMoveAbilityRequest.PredictionID, GetOwnerAbilityHandler()->FindActiveAbility(CustomMoveAbilityRequest.AbilityClass));
					CurrentRootMotionHandlers.Add(ExistingHandler);
					ReplicatedRootMotionHandlers.Add(ExistingHandler);
					//No delayed apply here, since the client should already have predicted this.
					ExistingHandler->Apply();
					ServerWaitingRootMotion.Remove(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
				}
			}
		}
		else
		{
			bUsingAbilityFromCustomMove = true;
			if (AbilityComponentRef->UseAbilityFromPredictedMovement(CustomMoveAbilityRequest))
			{
				//Document that this prediction ID has already been used, so duplicate moves with this ID do not get re-used.
				ServerCompletedMovementIDs.Add(FPredictedTick(CustomMoveAbilityRequest.PredictionID, CustomMoveAbilityRequest.Tick));
			}
			bUsingAbilityFromCustomMove = false;
		}
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//Check to see if the pending move has been marked as a misprediction, to prevent it from being replayed.
		if (!CompletedCastStatus.FindRef(PendingCustomMove.PredictionID))
		{
			return;
		}
		ExecuteCustomMove(PendingCustomMove.MoveParams);
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

void USaiyoraMovementComponent::ApplyCustomMove(UObject* Source, const FCustomMoveParams& CustomMove, USaiyoraRootMotionHandler* RootMotionHandler)
{
	if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
	{
		if (!IsValid(RootMotionHandler))
		{
			return;
		}
	}
	else if (CustomMove.MoveType == ESaiyoraCustomMove::None)
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
	if (!IsValid(SourceAsAbility) || SourceAsAbility->GetHandler()->GetOwner() != GetOwner() || SourceAsAbility->GetPredictionID() == 0)
	{
		//Not dealing with prediction.
		//Must happen on server.
		if (GetOwnerRole() != ROLE_Authority)
		{
			return;
		}
		//Avoid doubling up in the case of listen servers moving on Predicted and Server tick back to back. This also places a limit of one custom move per source per game tick.
		if (CurrentTickHandledMovement.Contains(Source))
		{
			return;
		}
		CurrentTickHandledMovement.Add(Source);
		if (PawnOwner->IsLocallyControlled())
		{
			if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
			{
				RootMotionHandler->Init(this, 0, Source);
				CurrentRootMotionHandlers.Add(RootMotionHandler);
				ReplicatedRootMotionHandlers.Add(RootMotionHandler);
				RootMotionHandler->Apply();
			}
			else
			{
				Multicast_ExecuteCustomMove(CustomMove);
			}
		}
		else
		{
			//Need to delay move by ping (up to a certain amount) to prevent jitter.
			//TODO: Cap ping, testing with no ping cap first.
			if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
			{
				RootMotionHandler->Init(this, 0, Source);
				HandlersAwaitingPingDelay.Add(RootMotionHandler);
				//Set timer to move handler from awaiting to replicated, add to current, then call Apply().
				const FTimerDelegate PingDelayDelegate = FTimerDelegate::CreateUObject(this, &USaiyoraMovementComponent::DelayedHandlerApplication, RootMotionHandler);
				//float const PingDelay = FMath::Min(MAXPINGDELAY, USaiyoraCombatLibrary::GetActorPing(GetOwner()));
				const float PingDelay = USaiyoraCombatLibrary::GetActorPing(GetOwner());
				GetWorld()->GetTimerManager().SetTimer(RootMotionHandler->PingDelayHandle, PingDelayDelegate, PingDelay, false);
				GetOwner()->ForceNetUpdate();
			}
			else
			{
				//Set timer to apply custom move after ping delay.
				const FTimerDelegate PingDelayDelegate = FTimerDelegate::CreateUObject(this, &USaiyoraMovementComponent::DelayedCustomMoveExecution, CustomMove);
				FTimerHandle PingDelayHandle;
				//const float PingDelay = FMath::Min(MAXPINGDELAY, USaiyoraCombatLibrary::GetActorPing(GetOwner()));
				const float PingDelay = USaiyoraCombatLibrary::GetActorPing(GetOwner());
				GetWorld()->GetTimerManager().SetTimer(PingDelayHandle, PingDelayDelegate, PingDelay, false);
				Client_ExecuteCustomMove(CustomMove);
			}
		}
	}
	else
	{
		//Dealing with prediction.
		switch (GetOwnerRole())
		{
		case ROLE_Authority :
			{
				if (bUsingAbilityFromCustomMove)
				{
					if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
					{
						RootMotionHandler->Init(this, SourceAsAbility->GetPredictionID(), Source);
						CurrentRootMotionHandlers.Add(RootMotionHandler);
						ReplicatedRootMotionHandlers.Add(RootMotionHandler);
						//No delayed apply here, since the client should already have predicted this.
						RootMotionHandler->Apply();
					}
					else
					{
						Multicast_ExecuteCustomMoveNoOwner(CustomMove);
					}
				}
				else
				{
					ServerCompletedMovementIDs.Add(FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick()));
					if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
					{
						ServerWaitingRootMotion.Add(FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick()), RootMotionHandler);
					}
					else
					{
						ServerWaitingCustomMoves.Add(FPredictedTick(SourceAsAbility->GetPredictionID(), SourceAsAbility->GetCurrentTick()), CustomMove);
					}
				}
			}
			break;
		case ROLE_AutonomousProxy :
			{
				SetupCustomMovementPrediction(SourceAsAbility, CustomMove);
				if (CustomMove.MoveType == ESaiyoraCustomMove::RootMotion)
				{
					RootMotionHandler->Init(this, SourceAsAbility->GetPredictionID(), Source);
					CurrentRootMotionHandlers.Add(RootMotionHandler);
					RootMotionHandler->Apply();
				}
			}
			break;
		default :
			return;
		}
	}
}

void USaiyoraMovementComponent::AddRootMotionHandlerFromReplication(USaiyoraRootMotionHandler* Handler)
{
	if (IsValid(Handler))
	{
		CurrentRootMotionHandlers.Add(Handler);
		ReplicatedRootMotionHandlers.Add(Handler);
	}
}

void USaiyoraMovementComponent::DelayedHandlerApplication(USaiyoraRootMotionHandler* Handler)
{
	if (!IsValid(Handler))
	{
		return;
	}
	if (HandlersAwaitingPingDelay.Remove(Handler) > 0)
	{
		CurrentRootMotionHandlers.Add(Handler);
		ReplicatedRootMotionHandlers.Add(Handler);
		Handler->Apply();
	}
}

void USaiyoraMovementComponent::RemoveRootMotionHandler(USaiyoraRootMotionHandler* Handler)
{
	//This function should ONLY be called by USaiyoraRootMotionHandler::Expire(). This is the cleanup function that actually removes.
	//It assumes that replication of bFinished and all PostExpire() functionality are already done.
	if (!IsValid(Handler))
	{
		return;
	}
	RemoveRootMotionSourceByID(Handler->GetHandledID());
	CurrentRootMotionHandlers.Remove(Handler);
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(Handler->PingDelayHandle))
		{
			HandlersAwaitingPingDelay.Remove(Handler);
			Handler->PingDelayHandle.Invalidate();
		}
		else
		{
			ReplicatedRootMotionHandlers.Remove(Handler);
		}
	}
}

void USaiyoraMovementComponent::ApplyJumpForce(UObject* Source, const ERootMotionAccumulateMode AccumulateMode,
	const int32 Priority, const float Duration, const FRotator& Rotation, const float Distance, const float Height,
	const bool bFinishOnLanded, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve, const bool bIgnoreRestrictions)
{
	UJumpForceHandler* JumpForce = NewObject<UJumpForceHandler>(GetOwner(), UJumpForceHandler::StaticClass());
	if (!IsValid(JumpForce))
	{
		return;
	}
	JumpForce->AccumulateMode = AccumulateMode;
	JumpForce->Priority = Priority;
	JumpForce->Duration = Duration;
	JumpForce->Rotation = Rotation;
	JumpForce->Distance = Distance;
	JumpForce->Height = Height;
	JumpForce->bFinishOnLanded = bFinishOnLanded;
	JumpForce->PathOffsetCurve = PathOffsetCurve;
	JumpForce->TimeMappingCurve = TimeMappingCurve;
	JumpForce->FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	JumpForce->bIgnoreRestrictions = bIgnoreRestrictions;
	//Can add the option to clamp velocity at the end I guess if needed.
	/*JumpForce->FinishSetVelocity = FVector::ZeroVector;
	JumpForce->FinishClampVelocity = 0.0f;*/
	//ApplyCustomRootMotionHandler(JumpForce, Source);
	FCustomMoveParams NewCustomMove = FCustomMoveParams();
	NewCustomMove.MoveType = ESaiyoraCustomMove::RootMotion;
	ApplyCustomMove(Source, NewCustomMove, JumpForce);
}

void USaiyoraMovementComponent::ApplyConstantForce(UObject* Source, const ERootMotionAccumulateMode AccumulateMode, const int32 Priority,
	const float Duration, const FVector& Force, UCurveFloat* StrengthOverTime, const bool bIgnoreRestrictions)
{
	UConstantForceHandler* ConstantForce = NewObject<UConstantForceHandler>(GetOwner(), UConstantForceHandler::StaticClass());
	if (!IsValid(ConstantForce))
	{
		return;
	}
	ConstantForce->AccumulateMode = AccumulateMode;
	ConstantForce->Priority = Priority;
	ConstantForce->Duration = Duration;
	ConstantForce->Force = Force;
	ConstantForce->StrengthOverTime = StrengthOverTime;
	ConstantForce->FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	ConstantForce->bIgnoreRestrictions = bIgnoreRestrictions;
	//ApplyCustomRootMotionHandler(ConstantForce, Source);
	FCustomMoveParams NewCustomMove = FCustomMoveParams();
	NewCustomMove.MoveType = ESaiyoraCustomMove::RootMotion;
	ApplyCustomMove(Source, NewCustomMove, ConstantForce);
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
			TArray<USaiyoraRootMotionHandler*> HandlersToRemove = CurrentRootMotionHandlers;
			HandlersToRemove.Append(HandlersAwaitingPingDelay);
			for (USaiyoraRootMotionHandler* Handler : HandlersToRemove)
			{
				if (!Handler->bIgnoreRestrictions)
				{
					Handler->CancelRootMotion();
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

void USaiyoraMovementComponent::OnRep_MovementRestricted()
{
	if (bExternalMovementRestricted)
	{
		TArray<USaiyoraRootMotionHandler*> Handlers = CurrentRootMotionHandlers;
		for (USaiyoraRootMotionHandler* Handler : Handlers)
		{
			if (!Handler->bIgnoreRestrictions)
			{
				Handler->CancelRootMotion();
			}
		}
	}
}

void USaiyoraMovementComponent::StopMotionOnOwnerDeath(AActor* Target, const ELifeStatus Previous,
	const ELifeStatus New)
{
	if (Target == GetOwner() && New != ELifeStatus::Alive)
	{
		TArray<USaiyoraRootMotionHandler*> HandlersToRemove = CurrentRootMotionHandlers;
		if (GetOwnerRole() == ROLE_Authority)
		{
			HandlersToRemove.Append(HandlersAwaitingPingDelay);
		}
		for (USaiyoraRootMotionHandler* Handler : HandlersToRemove)
		{
			if (IsValid(Handler))
			{
				Handler->CancelRootMotion();
			}
		}
		StopMovementImmediately();
	}
}

void USaiyoraMovementComponent::StopMotionOnRooted(const FCrowdControlStatus& Previous, const FCrowdControlStatus& New)
{
	//TODO: Can add ping delay to root taking effect for auto proxies? How does this work with sim proxies?
	if (New.CrowdControlType == FSaiyoraCombatTags::Get().Cc_Root && New.bActive)
	{
		TArray<USaiyoraRootMotionHandler*> HandlersToRemove = CurrentRootMotionHandlers;
		if (GetOwnerRole() == ROLE_Authority)
		{
			HandlersToRemove.Append(HandlersAwaitingPingDelay);
		}
		for (USaiyoraRootMotionHandler* Handler : HandlersToRemove)
		{
			if (IsValid(Handler) && !Handler->bIgnoreRestrictions)
			{
				Handler->CancelRootMotion();
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