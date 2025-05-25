#include "NPCAbilityComponent.h"

#include "CombatDebugOptions.h"
#include "CombatLink.h"
#include "DamageHandler.h"
#include "NPCAbility.h"
#include "NPCSubsystem.h"
#include "PatrolRoute.h"
#include "RotationBehaviors.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "UnrealNetwork.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Navigation/PathFollowingComponent.h"

#pragma region Initialization

UNPCAbilityComponent::UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bWantsInitializeComponent = true;
}

void UNPCAbilityComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	ThreatHandlerRef = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
	MovementComponentRef = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(GetOwner());
}

void UNPCAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	OwnerAsPawn = Cast<APawn>(GetOwner());
	checkf(IsValid(OwnerAsPawn), TEXT("NPC Ability Component's owner was not a valid pawn."));
	DungeonGameStateRef = Cast<ADungeonGameState>(GetGameStateRef());
	checkf(IsValid(DungeonGameStateRef), TEXT("Got an invalid Game State Ref in NPC Ability Component."));
	NPCSubsystemRef = GetWorld()->GetSubsystem<UNPCSubsystem>();
	checkf(IsValid(NPCSubsystemRef), TEXT("Invalid NPCSubsystem ref in NPC Ability Component."));

	OwnerAsPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UNPCAbilityComponent::OnControllerChanged);
	DungeonGameStateRef->OnDungeonPhaseChanged.AddDynamic(this, &UNPCAbilityComponent::OnDungeonPhaseChanged);
	if (IsValid(DamageHandlerRef))
	{
		DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UNPCAbilityComponent::OnLifeStatusChanged);
	}
	if (IsValid(ThreatHandlerRef))
	{
		ThreatHandlerRef->OnCombatChanged.AddDynamic(this, &UNPCAbilityComponent::OnCombatChanged);
	}
	OnControllerChanged(OwnerAsPawn, nullptr, OwnerAsPawn->GetController());

	//Copy the patrol information out of the patrol route actor into a series of structs we can use.
	if (IsValid(PatrolRoute))
	{
		PatrolRoute->GetPatrolRoute(PatrolPath);
	}
}

void UNPCAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickRotation(DeltaTime);
}

void UNPCAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPCAbilityComponent, CombatBehavior);
}

void UNPCAbilityComponent::OnControllerChanged(APawn* PossessedPawn, AController* OldController,
	AController* NewController)
{
	//Remove any move request delegates from the previous controller.
	if (IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	}
	
	//Actually update the controller reference here.
	AIController = Cast<AAIController>(NewController);

	//Bind to the move request delegate for the new controller.
	if (IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &UNPCAbilityComponent::OnMoveRequestFinished);
	}
	SetWantsToMove(true);
	UpdateCombatBehavior();
}

#pragma endregion
#pragma region State Control

void UNPCAbilityComponent::UpdateCombatBehavior()
{
	ENPCCombatBehavior NewCombatBehavior = ENPCCombatBehavior::None;
	if (DungeonGameStateRef->GetDungeonPhase() == EDungeonPhase::InProgress)
	{
		if (IsValid(AIController))
		{
			if (!IsValid(DamageHandlerRef) || DamageHandlerRef->GetLifeStatus() == ELifeStatus::Alive)
			{
				if (IsValid(ThreatHandlerRef) && ThreatHandlerRef->IsInCombat())
				{
					NewCombatBehavior = ENPCCombatBehavior::Combat;
				}
				else
				{
					NewCombatBehavior = bNeedsReset ? ENPCCombatBehavior::Resetting : ENPCCombatBehavior::Patrolling;
				}
			}
		}
	}
	
	if (NewCombatBehavior != CombatBehavior)
	{
		//Actually update the combat behavior.
		const ENPCCombatBehavior PreviousBehavior = CombatBehavior;
		CombatBehavior = NewCombatBehavior;
		switch(PreviousBehavior)
		{
		case ENPCCombatBehavior::Combat :
			LeaveCombatState();
			break;
		case ENPCCombatBehavior::Patrolling :
			LeavePatrolState();
			break;
		case ENPCCombatBehavior::Resetting :
			LeaveResetState();
			break;
		default:
			break;
		}
		
		switch (CombatBehavior)
		{
		case ENPCCombatBehavior::Combat :
			EnterCombatState();
			break;
		case ENPCCombatBehavior::Patrolling :
			EnterPatrolState();
			break;
		case ENPCCombatBehavior::Resetting :
			EnterResetState();
			break;
		default:
			break;
		}
		OnCombatBehaviorChanged.Broadcast(PreviousBehavior, CombatBehavior);
	}
}

void UNPCAbilityComponent::OnRep_CombatBehavior(const ENPCCombatBehavior Previous)
{
	OnCombatBehaviorChanged.Broadcast(Previous, CombatBehavior);
}

#pragma endregion
#pragma region Move Requests

void UNPCAbilityComponent::SetMoveGoal(const FVector& Location)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (bHasValidMoveLocation && Location == CurrentMoveLocation)
	{
		return;
	}

	CurrentMoveLocation = Location;
	bHasValidMoveLocation = true;
	if (CombatBehavior == ENPCCombatBehavior::Combat)
	{
		NPCSubsystemRef->ClaimLocation(GetOwner(), CurrentMoveLocation);
	}

	if (bWantsToMove)
	{
		TryMoveToGoal();
	}
}

void UNPCAbilityComponent::SetWantsToMove(const bool bNewMove)
{
	const bool bShouldActuallyMove = bNewMove && IsValid(AIController) && IsValid(MovementComponentRef);
	if (bShouldActuallyMove == bWantsToMove)
	{
		return;
	}
	bWantsToMove = bShouldActuallyMove;
	if (bWantsToMove)
	{
		if (bHasValidMoveLocation)
		{
			TryMoveToGoal();
		}
	}
	else
	{
		AbortActiveMove();
	}
}

void UNPCAbilityComponent::TryMoveToGoal()
{
	//Request a move to the queried location.
	//TODO: These parameters might need to be adjusted, I don't know what some of them do.
	FAIMoveRequest MoveReq(CurrentMoveLocation);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(false);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
	MoveReq.SetAcceptanceRadius(-1.0f);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);
	
	CurrentMoveRequestID = AIController->MoveTo(MoveReq).MoveId;
	CurrentMoveRequestBehavior = CombatBehavior;
}

void UNPCAbilityComponent::AbortActiveMove()
{
	//If we have a move in progress...
	if (CurrentMoveRequestID.IsValid())
	{
		//If the AI controller and PathFollowingComponent are both valid, we can actually just abort the move.
		//This will call OnMoveRequestFinished, and clear variables for us.
		if (IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
		{
			AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, CurrentMoveRequestID);
		}
		//If something wasn't valid, we'll just clear the variables here.
		else
		{
			CurrentMoveRequestID = FAIRequestID::InvalidRequest;
			CurrentMoveRequestBehavior = ENPCCombatBehavior::None;
		}
	}
}

void UNPCAbilityComponent::OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& PathResult)
{
	const bool bIDMatched = RequestID == CurrentMoveRequestID;
	const bool bBehaviorMatched = CombatBehavior == CurrentMoveRequestBehavior;
	CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	CurrentMoveRequestBehavior = ENPCCombatBehavior::None;

	const bool bAlreadyAtGoal = PathResult.Flags & FPathFollowingResultFlags::AlreadyAtGoal;
	const bool bAbortedByOwner = PathResult.Flags & FPathFollowingResultFlags::OwnerFinished;

	if (bIDMatched)
	{
		//If we changed behaviors during this move request, we do not care that it finished.
		if (!bBehaviorMatched)
		{
			return;
		}
		if (PathResult.Code == EPathFollowingResult::Success || bAlreadyAtGoal)
		{
			//If we successfully reached our goal, we can move to the next patrol location or finish our reset.
			switch (CombatBehavior)
			{
			case ENPCCombatBehavior::Patrolling :
				OnReachedPatrolPoint();
				break;
			case ENPCCombatBehavior::Resetting :
				bNeedsReset = false;
				UpdateCombatBehavior();
				break;
			default :
				return;
			}
		}
		else if (bAbortedByOwner)
		{
			//If the move was aborted by us, we don't need to handle anything.
		}
		else
		{
			//For patrolling, we will just continue moving. Probably want to implement a counter so that if we just get permanently stuck we don't loop forever.
			//For resetting, we just teleport to the reset location and leave reset state.
			switch (CombatBehavior)
			{
			case ENPCCombatBehavior::Patrolling :
				OnReachedPatrolPoint();
				break;
			case ENPCCombatBehavior::Resetting :
				bNeedsReset = false;
				GetOwner()->SetActorLocation(ResetGoal);
				UpdateCombatBehavior();
				break;
			default :
				return;
			}
		}
	}
}

#pragma endregion
#pragma region Rotation

FRotationLock UNPCAbilityComponent::LockRotation()
{
	const FRotationLock& NewLock = RotationLocks.Add_GetRef(FRotationLock::GenerateRotationLock());
	return NewLock;
}

void UNPCAbilityComponent::UnlockRotation(const FRotationLock& Lock)
{
	RotationLocks.Remove(Lock);
}

void UNPCAbilityComponent::TickRotation(const float DeltaTime)
{
	//If rotation is locked, don't rotate.
	if (RotationLocks.Num() > 0)
	{
		return;
	}
	FNPCRotationBehavior* RotationBehavior = DefaultRotationBehavior.GetMutablePtr<FNPCRotationBehavior>();
	if (!RotationBehavior)
	{
		//If we don't have a valid rotation behavior, we can just use control rotation yaw as a reasonable default.
		if (!AIController->GetPawn()->bUseControllerRotationYaw)
		{
			AIController->GetPawn()->bUseControllerRotationYaw = true;
		}
		return;
	}
	//Initialize the rotation behavior if we haven't already.
	if (!RotationBehavior->IsInitialized())
	{
		RotationBehavior->Initialize(GetOwner());
	}
	//If we are using control rotation, disable that so that we can freely override rotation.
	if (AIController->GetPawn()->bUseControllerRotationYaw)
	{
		AIController->GetPawn()->bUseControllerRotationYaw = false;
	}
	if (AIController->GetPawn()->bUseControllerRotationPitch)
	{
		AIController->GetPawn()->bUseControllerRotationPitch = false;
	}
	if (AIController->GetPawn()->bUseControllerRotationRoll)
	{
		AIController->GetPawn()->bUseControllerRotationRoll = false;
	}
	
	const FRotator PreviousRotation = GetOwner()->GetActorRotation();
	FRotator OwnerRotation = PreviousRotation;
	FRotator UnclampedRotation = OwnerRotation;
	RotationBehavior->ModifyRotation(DeltaTime, GetOwner(), OwnerRotation, UnclampedRotation);
	GetOwner()->SetActorRotation(OwnerRotation);

	if (CombatDebugOptions->bDrawRotationBehavior)
	{
		FRotationDebugInfo DebugInfo;
		DebugInfo.OriginalRotation = PreviousRotation;
		DebugInfo.bClampingRotation = RotationBehavior->bEnforceRotationSpeed;
		DebugInfo.UnclampedRotation = UnclampedRotation;
		DebugInfo.FinalRotation = OwnerRotation;
		CombatDebugOptions->DrawRotationBehavior(GetOwner(), DebugInfo);
	}
}

#pragma endregion
#pragma region Query

void UNPCAbilityComponent::SetQuery(UEnvQuery* Query, const TArray<FInstancedStruct>& Params)
{
	if (!IsValid(Query))
	{
		return;
	}
	AbortActiveQuery();
	CurrentQuery = Query;
	CurrentQueryParams.Empty();
	for (const FInstancedStruct& Param : Params)
	{
		if (Param.GetPtr<FNPCQueryParam>())
		{
			CurrentQueryParams.Add(Param);
		}
	}
	if (GetWorld()->GetTimerManager().IsTimerActive(QueryRetryHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(QueryRetryHandle);
	}
	RunQuery();
}

void UNPCAbilityComponent::RunQuery()
{
	if (!IsValid(CurrentQuery))
	{
		QueryID = INDEX_NONE;
		return;
	}
	FEnvQueryRequest Request (CurrentQuery);
	for (const FInstancedStruct& InstancedParam : CurrentQueryParams)
	{
		const FNPCQueryParam* Param = InstancedParam.GetPtr<FNPCQueryParam>();
		if (!Param)
		{
			continue;
		}
		Param->AddParam(Request);
	}
	LastQueryBehavior = CombatBehavior;
	QueryID = Request.Execute(EEnvQueryRunMode::SingleResult, this, &UNPCAbilityComponent::OnQueryFinished);
	if (QueryID == INDEX_NONE)
	{
		GetWorld()->GetTimerManager().SetTimer(QueryRetryHandle, this, &UNPCAbilityComponent::RunQuery, GetQueryRetryDelay());
	}
}

void UNPCAbilityComponent::OnQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult)
{
	//Ignore all query results and do not retry if we have changed behaviors.
	if (CombatBehavior != LastQueryBehavior)
	{
		return;
	}
	const FEnvQueryResult* Result = QueryResult.IsValid() ? QueryResult.Get() : nullptr;
	if (Result)
	{
		//If we have overwritten this query with another query, ignore the result.
		if (Result->QueryID != QueryID)
		{
			return;
		}
		//If we aborted this query manually, ignore the result.
		if (Result->IsAborted())
		{
			return;
		}
		//If we got a valid location from the query, update our move goal.
		if (Result->IsFinished() && Result->IsSuccessful() && Result->Items.Num() > 0 && Result->ItemType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
		{
			SetMoveGoal(Result->GetItemAsLocation(0));
		}
	}

	//Reset variable and start the retry timer for the next query.
	QueryID = INDEX_NONE;
	LastQueryBehavior = ENPCCombatBehavior::None;
	GetWorld()->GetTimerManager().SetTimer(QueryRetryHandle, this, &UNPCAbilityComponent::RunQuery, GetQueryRetryDelay());
}

void UNPCAbilityComponent::AbortActiveQuery()
{
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(GetWorld());
		if (IsValid(QueryManager))
		{
			QueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}
}

#pragma endregion 
#pragma region Combat

void UNPCAbilityComponent::EnterCombatState()
{
	SetComponentTickEnabled(true);
	NPCSubsystemRef->ClaimLocation(GetOwner(), GetOwner()->GetActorLocation());
	InitCombatChoices();
	if (CombatPriority.Num() > 0)
	{
		TrySelectNewChoice();
	}
	QueryRetryRandomSeed = GenerateQueryRetryRandomSeed();
	SetQuery(DefaultQuery, DefaultQueryParams);
}

void UNPCAbilityComponent::InitCombatChoices()
{
	for (FNPCCombatChoice& CombatChoice : CombatPriority)
	{
		CombatChoice.Init(this);
	}
}

void UNPCAbilityComponent::TrySelectNewChoice()
{
	bWaitingOnMovementStop = false;
	QueuedChoiceIdx = -1;
	if (GetWorld()->GetTimerManager().IsTimerActive(ChoiceRetryHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(ChoiceRetryHandle);
	}
	//Find the highest priority choice.
	int ChoiceIdx = -1;
	for (int i = 0; i < CombatPriority.Num(); i++)
	{
		if (CombatPriority[i].IsChoiceValid())
		{
			ChoiceIdx = i;
			break;
		}
	}

	if (ChoiceIdx == -1)
	{
		//If we failed to find a valid choice, we will try again in a bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		return;
	}

	UNPCAbility* AbilityInstance = Cast<UNPCAbility>(FindActiveAbility(CombatPriority[ChoiceIdx].GetAbilityClass()));
	if (!IsValid(AbilityInstance))
	{
		//If we failed to find a valid choice, we will try again in a bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		return;
	}
	if (!AbilityInstance->IsCastableWhileMoving())
	{
		SetWantsToMove(false);
		//If we're moving, and we don't want to be, we have to wait for the movement component to finish moving before using the ability.
		if (MovementComponentRef->IsMoving())
		{
			//Since we have to wait before using the ability, we will reserve the ability token so no other NPCs can use it while we're waiting.
			if (AbilityInstance->UsesTokens())
			{
				const bool bGotToken = NPCSubsystemRef->RequestAbilityToken(AbilityInstance, true);
				if (!bGotToken)
				{
					//If we failed to get a token for the ability, we will try again in a bit.
					GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
					return;
				}
				PossessedToken = AbilityInstance;
			}
			bWaitingOnMovementStop = true;
			QueuedChoiceIdx = ChoiceIdx;
			MovementComponentRef->OnMovementChanged.AddDynamic(this, &UNPCAbilityComponent::TryUseQueuedAbility);
			return;
		}
	}
	const FAbilityEvent AbilityEvent = UseAbility(CombatPriority[ChoiceIdx].GetAbilityClass());
	if (AbilityEvent.ActionTaken == ECastAction::Success && AbilityInstance->GetCastType() == EAbilityCastType::Channel)
	{
		//Bind to the cast end delegate so that we can select a new choice when this ability ends.
		OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::EndChoiceOnCastStateChanged);
	}
	else
	{
		//After casting this ability, we'll select another choice in a little bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		SetWantsToMove(true);
	}
}

void UNPCAbilityComponent::TryUseQueuedAbility(AActor* Actor, const bool bNewMovement)
{
	if (bNewMovement)
	{
		return;
	}
	MovementComponentRef->OnMovementChanged.RemoveDynamic(this, &UNPCAbilityComponent::TryUseQueuedAbility);
	if (!bWaitingOnMovementStop)
	{
		if (IsValid(PossessedToken))
		{
			NPCSubsystemRef->ReturnAbilityToken(PossessedToken);
			PossessedToken = nullptr;
		}
		return;
	}
	const int Idx = QueuedChoiceIdx;
	QueuedChoiceIdx = -1;
	bWaitingOnMovementStop = false;
	if (!CombatPriority.IsValidIndex(Idx))
	{
		SetWantsToMove(true);
		return;
	}
	const FNPCCombatChoice& Choice = CombatPriority[Idx];
	const FAbilityEvent AbilityEvent = UseAbility(Choice.GetAbilityClass());
	const UCombatAbility* AbilityInstance = FindActiveAbility(Choice.GetAbilityClass());
	if (AbilityEvent.ActionTaken == ECastAction::Success && AbilityInstance->GetCastType() == EAbilityCastType::Channel)
	{
		//Bind to the cast end delegate so that we can select a new choice when this ability ends.
		OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::EndChoiceOnCastStateChanged);
	}
	else
	{
		//If the ability failed to cast, we need to return the ability token we had reserved.
		if (AbilityEvent.ActionTaken == ECastAction::Fail && IsValid(PossessedToken))
		{
			NPCSubsystemRef->ReturnAbilityToken(PossessedToken);
			PossessedToken = nullptr;
		}
		//After casting this ability, we'll select another choice in a little bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		SetWantsToMove(true);
	}
}

void UNPCAbilityComponent::EndChoiceOnCastStateChanged(const FCastingState& Previous, const FCastingState& New)
{
	if (!New.bIsCasting)
	{
		OnCastStateChanged.RemoveDynamic(this, &UNPCAbilityComponent::EndChoiceOnCastStateChanged);
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		SetWantsToMove(true);
	}
}

void UNPCAbilityComponent::LeaveCombatState()
{
	SetComponentTickEnabled(false);
	//If we had reserved a token for an ability, release the token so other NPCs can use it.
	if (IsValid(PossessedToken))
	{
		NPCSubsystemRef->ReturnAbilityToken(PossessedToken);
		PossessedToken = nullptr;
	}
	bWaitingOnMovementStop = false;
	QueuedChoiceIdx = -1;
	if (IsCasting())
	{
		CancelCurrentCast();
	}
	AbortActiveQuery();
	//Stop the query retry timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(QueryRetryHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(QueryRetryHandle);
	}
	//Stop the choice selection retry timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(ChoiceRetryHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(ChoiceRetryHandle);
	}
	NPCSubsystemRef->FreeLocation(GetOwner());
}

#pragma endregion
#pragma region Patrolling

void UNPCAbilityComponent::EnterPatrolState()
{
	//Apply patrol move speed mod.
	if (PatrolMoveSpeedModifier > 0.0f)
	{
		if (IsValid(StatHandlerRef))
		{
			PatrolMoveSpeedModHandle = StatHandlerRef->AddStatModifier(FSaiyoraCombatTags::Get().Stat_MaxWalkSpeed, FCombatModifier(PatrolMoveSpeedModifier, EModifierType::Multiplicative));
		}
		else if (IsValid(MovementComponentRef))
		{
			//This could easily go wrong if anything else modifies max walk speed while the mob is patrolling. Maybe just don't let this happen?
			DefaultMaxWalkSpeed = MovementComponentRef->GetDefaultMaxWalkSpeed();
			MovementComponentRef->MaxWalkSpeed = DefaultMaxWalkSpeed * PatrolMoveSpeedModifier;
		}
	}
	//If we have a patrol path and it isn't already finished, we want to start moving to the next point.
	if (PatrolPath.IsValidIndex(NextPatrolIndex))
	{
		MoveToNextPatrolPoint();
	}
	//If we aren't going to move, we mark the patrol as finished.
	else
	{
		FinishPatrol();
	}
}

void UNPCAbilityComponent::LeavePatrolState()
{
	//Clear the patrol move speed modifier.
	if (PatrolMoveSpeedModHandle.IsValid())
	{
		if (IsValid(StatHandlerRef))
		{
			StatHandlerRef->RemoveStatModifier(FSaiyoraCombatTags::Get().Stat_MaxWalkSpeed, PatrolMoveSpeedModHandle);
			PatrolMoveSpeedModHandle = FCombatModifierHandle::Invalid;
		}
		else if (IsValid(MovementComponentRef) && DefaultMaxWalkSpeed > 0.0f)
		{
			MovementComponentRef->MaxWalkSpeed = DefaultMaxWalkSpeed;
			DefaultMaxWalkSpeed = 0.0f;
		}
	}
	//Save off our current location for resetting to when combat ends.
	ResetGoal = GetOwner()->GetActorLocation();
	bNeedsReset = true;
	//Clear our patrol wait timer, if we were waiting at a patrol point.
	if (GetWorld()->GetTimerManager().IsTimerActive(PatrolWaitHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(PatrolWaitHandle);
	}
}

void UNPCAbilityComponent::SetPatrolSubstate(const ENPCPatrolSubstate NewSubstate)
{
	PatrolSubstate = NewSubstate;
	OnPatrolStateChanged.Broadcast(GetOwner(), PatrolSubstate);
}

void UNPCAbilityComponent::MoveToNextPatrolPoint()
{
	//If we have finished the patrol, nothing more to do.
	if (!PatrolPath.IsValidIndex(NextPatrolIndex))
	{
		FinishPatrol();
		return;
	}
	
	SetPatrolSubstate(ENPCPatrolSubstate::MovingToPoint);
	if (IsValid(AIController))
	{
		//Request a move to the next patrol location from the AIController.
		SetMoveGoal(PatrolPath[NextPatrolIndex].Location);
	}
	else
	{
		FinishPatrol();
	}
}

void UNPCAbilityComponent::OnReachedPatrolPoint()
{
	LastPatrolIndex = NextPatrolIndex;
	if (PatrolPath[NextPatrolIndex].WaitTime > 0.0f)
	{
		//Set a timer to wait at this patrol location for a duration.
		//We don't increment the patrol index yet because if we enter combat during this wait, we don't want to skip it and move to the next point yet.
		GetWorld()->GetTimerManager().SetTimer(PatrolWaitHandle, this, &UNPCAbilityComponent::FinishPatrolWait, PatrolPath[NextPatrolIndex].WaitTime);
		SetPatrolSubstate(ENPCPatrolSubstate::WaitingAtPoint);
	}
	else
	{
		OnPatrolLocationReached.Broadcast(GetOwner(), GetOwner()->GetActorLocation());
		if (!bGroupPatrolling)
		{
			IncrementPatrolIndex();
			MoveToNextPatrolPoint();
		}
	}
}

void UNPCAbilityComponent::FinishPatrolWait()
{
	OnPatrolLocationReached.Broadcast(GetOwner(), GetOwner()->GetActorLocation());
	if (!bGroupPatrolling)
	{
		IncrementPatrolIndex();
		MoveToNextPatrolPoint();
	}
}

void UNPCAbilityComponent::IncrementPatrolIndex()
{
	//If we're walking forward along our patrol route, check if this point is the end of the patrol.
	if (!bReversePatrolling)
	{
		//If we're hitting the end of the loop, we either turn around or we go back to the start.
		if (NextPatrolIndex == PatrolPath.Num() - 1 && bLoopPatrol)
		{
			if (bLoopReverse)
			{
				NextPatrolIndex--;
				bReversePatrolling = true;
			}
			else
			{
				NextPatrolIndex = 0;
			}
		}
		//Otherwise, on to the next patrol point in the line.
		else
		{
			NextPatrolIndex++;
		}
	}
	//If we're walking backward along our patrol route, check if we reached the start of the patrol.
	else
	{
		//If we've hit the start of the loop, we need to turn around again.
		if (NextPatrolIndex == 0)
		{
			//We know that we're looping by default if we ended up here, so no need to check.
			NextPatrolIndex++;
			bReversePatrolling = false;
		}
		//Otherwise, keep walking backwards.
		else
		{
			NextPatrolIndex--;
		}
	}
}

void UNPCAbilityComponent::FinishPatrol()
{
	SetPatrolSubstate(ENPCPatrolSubstate::PatrolFinished);
}

void UNPCAbilityComponent::SetupGroupPatrol(ACombatLink* LinkActor)
{
	if (!GetOwner()->HasAuthority() || !IsValid(LinkActor))
	{
		return;
	}
	bGroupPatrolling = true;
	GroupLink = LinkActor;
	LinkActor->OnPatrolSegmentCompleted.AddDynamic(this, &UNPCAbilityComponent::FinishGroupPatrolSegment);
}

void UNPCAbilityComponent::FinishGroupPatrolSegment()
{
	if (CombatBehavior == ENPCCombatBehavior::Patrolling && PatrolSubstate != ENPCPatrolSubstate::PatrolFinished)
	{
		IncrementPatrolIndex();
		MoveToNextPatrolPoint();
	}
}

#pragma endregion
#pragma region Resetting

void UNPCAbilityComponent::EnterResetState()
{
	if (!IsValid(AIController) || FVector::DistSquared(GetOwner()->GetActorLocation(), ResetGoal) > FMath::Square(ResetTeleportDistance))
	{
		GetOwner()->SetActorLocation(ResetGoal);
		bNeedsReset = false;
		UpdateCombatBehavior();
	}
	else
	{
		//Request a move to the next patrol location from the AIController.
		//TODO: These parameters might need to be adjusted, I don't know what some of them do.
		FAIMoveRequest MoveReq(ResetGoal);
		MoveReq.SetUsePathfinding(true);
		MoveReq.SetAllowPartialPath(false);
		MoveReq.SetProjectGoalLocation(true);
		MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
		MoveReq.SetAcceptanceRadius(-1.0f);
		MoveReq.SetReachTestIncludesAgentRadius(true);
		MoveReq.SetCanStrafe(true);
		const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveReq);
		if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
		{
			CurrentMoveRequestID = RequestResult.MoveId;
			CurrentMoveRequestBehavior = CombatBehavior;
		}
		//If we were already at the reset goal, we can immediately move to patrol state.
		else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			bNeedsReset = false;
			UpdateCombatBehavior();
		}
		//If the path following result failed, move to the next point.
		else
		{
			GetOwner()->SetActorLocation(ResetGoal);
			bNeedsReset = false;
			UpdateCombatBehavior();
		}
	}
}

void UNPCAbilityComponent::LeaveResetState()
{
	//If I need any specific behavior not handled by other components it can go here.
}

#pragma endregion 