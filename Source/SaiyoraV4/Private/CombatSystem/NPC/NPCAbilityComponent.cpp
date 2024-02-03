#include "NPCAbilityComponent.h"
#include "DamageHandler.h"
#include "NPCAbility.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "UnrealNetwork.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Navigation/PathFollowingComponent.h"

#pragma region Initialization

UNPCAbilityComponent::UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//We tick this component so that ability conditions that need to update on tick can be updated.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bWantsInitializeComponent = true;
}

void UNPCAbilityComponent::InitializeComponent()
{
	Super::InitializeComponent();
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
	DungeonGameStateRef = Cast<ADungeonGameState>(GameStateRef);
	checkf(IsValid(GameStateRef), TEXT("Got an invalid Game State Ref in NPC Ability Component."));

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
	InitializeCombatPriorities();
}

void UNPCAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPCAbilityComponent, CombatBehavior);
}

#pragma endregion
#pragma region State Control

void UNPCAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Some ability and movement conditions might need to update on tick (timing, distance checking), so we will tick them.
	for (FNPCAbilityChoice& Choice : AbilityPriority)
	{
		Choice.TickConditions(GetOwner(), DeltaTime);
	}
	for (FNPCMovementChoice& Choice : MovementPriority)
	{
		Choice.TickConditions(GetOwner(), DeltaTime);
	}
}

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
		const ENPCCombatBehavior PreviousBehavior = CombatBehavior;
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
		CombatBehavior = NewCombatBehavior;
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

void UNPCAbilityComponent::OnControllerChanged(APawn* PossessedPawn, AController* OldController,
	AController* NewController)
{
	if (IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	}
	AIController = Cast<AAIController>(NewController);
	if (IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &UNPCAbilityComponent::OnMoveRequestFinished);
	}
	UpdateCombatBehavior();
}

#pragma endregion
#pragma region Move Requests

void UNPCAbilityComponent::OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& PathResult)
{
	const bool bIDMatched = RequestID == CurrentMoveRequestID;
	CurrentMoveRequestID = FAIRequestID::InvalidRequest;

	const bool bAlreadyAtGoal = PathResult.Flags & FPathFollowingResultFlags::AlreadyAtGoal;
	const bool bAbortedByOwner = PathResult.Flags & FPathFollowingResultFlags::OwnerFinished;

	if (bIDMatched)
	{
		if (PathResult.Code == EPathFollowingResult::Success || bAlreadyAtGoal)
		{
			//If we successfully reached our goal, we can move to the next patrol location, finish our rest, or execute our chosen ability.
			switch (CombatBehavior)
			{
			case ENPCCombatBehavior::Patrolling :
				OnReachedPatrolPoint();
				break;
			case ENPCCombatBehavior::Resetting :
				bNeedsReset = false;
				UpdateCombatBehavior();
				break;
			case ENPCCombatBehavior::Combat :
				ExecuteAbilityChoice();
				break;
			default :
				return;
			}
		}
		else if (bAbortedByOwner)
		{
			//This means we are already planning for this move to fail/end early, so nothing needs to be handled.
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC %s did not succeed in move request. State was %s, result was %s."),
				*GetOwner()->GetName(), *UEnum::GetDisplayValueAsText(CombatBehavior).ToString(), *UEnum::GetDisplayValueAsText(PathResult.Code).ToString());

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
			case ENPCCombatBehavior::Combat :
				AbortCurrentAbilityChoice();
				break;
			default :
				return;
			}
		}
	}
}

#pragma endregion 
#pragma region Combat

void UNPCAbilityComponent::InitializeCombatPriorities()
{
	for (int i = AbilityPriority.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(AbilityPriority[i].AbilityClass))
		{
			AbilityPriority.RemoveAt(i);
			continue;
		}
		//Learn the ability associated with this choice.
		AbilityPriority[i].AbilityInstance = Cast<UNPCAbility>(AddNewAbility(AbilityPriority[i].AbilityClass));
		if (!IsValid(AbilityPriority[i].AbilityInstance))
		{
			//If we failed to learn the ability, its possible we already know it.
			//This is because the same ability can be in the priority list multiple times behind different conditions.
			//Check here if we already know the ability.
			AbilityPriority[i].AbilityInstance = Cast<UNPCAbility>(FindActiveAbility(AbilityPriority[i].AbilityClass));
			if (!IsValid(AbilityPriority[i].AbilityInstance))
			{
				AbilityPriority.RemoveAt(i);
				continue;
			}
		}
		AbilityPriority[i].ChoiceIndex = i;
		//If this choice is high priority, bind to its delegate to be notified when it becomes available.
		//If it becomes available while we are preparing for or executing a lower priority ability, we'll abort and execute this choice instead.
		if (AbilityPriority[i].bHighPriority)
		{
			AbilityPriority[i].OnChoiceAvailable.BindUObject(this, &UNPCAbilityComponent::OnAbilityChoiceAvailable);
		}
		AbilityPriority[i].SetupConditions(GetOwner());
	}
	
	for (int i = MovementPriority.Num() - 1; i >= 0; i--)
	{
		MovementPriority[i].ChoiceIndex = i;
		//If this choice is high priority, bind to its delegate to be notified when it becomes available.
		//If it becomes available while doing a lower priority movement, we'll abort and execute this choice instead.
		if (MovementPriority[i].bHighPriority)
		{
			MovementPriority[i].OnChoiceAvailable.BindUObject(this, &UNPCAbilityComponent::OnMovementChoiceAvailable);
		}
		MovementPriority[i].SetupConditions(GetOwner());
	}
}

void UNPCAbilityComponent::EnterCombatState()
{
	//If we want to use a custom combat tree, we just run that behavior tree on entering the state.
	if (bUseCustomCombatTree)
	{
		if (!IsValid(CombatTree) || !IsValid(AIController) || !AIController->RunBehaviorTree(CombatTree))
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC %s failed to run a custom combat tree."), *GetOwner()->GetName());
		}
	}
	//Otherwise, we will initiate the default ability/movement priority selection process.
	else
	{
		//We want to be notified when casts end to select new abilities.
		OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::OnCastChanged);
		//If we are casting, we'll get a callback from the above function, otherwise, we can just go straight into ability selection.
		if (!IsCasting())
		{
			SelectAbilityChoice();
		}
	}
}

void UNPCAbilityComponent::SelectAbilityChoice()
{
	//Cache off what ability choice we were already using, if any.
	//This should usually be -1 unless we are selecting because a high priority ability became available.
	const int32 PreviousAbilityChoice = CurrentAbilityChoice;
	const bool bHadPreviousSelection = PreviousAbilityChoice != -1;
	int32 NewChoice = -1;
	for (int i = 0; i < AbilityPriority.Num(); i++)
	{
		//Select the first choice where all conditions pass and the ability is usable.
		ECastFailReason FailReason;
		if (AbilityPriority[i].AreConditionsMet() && CanUseAbility(AbilityPriority[i].AbilityInstance, FailReason))
		{
			NewChoice = i;
			UE_LOG(LogTemp, Warning, TEXT("%s: Selected ability choice index %i with ability class %s at time %f"),
				*GetOwner()->GetName(), i, *AbilityPriority[CurrentAbilityChoice].AbilityClass->GetName(), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
			break;
		}
	}
	//Check if we failed to find a good choice or we picked a lower priority (higher index) choice than what was already happening.
	if (NewChoice == -1 || (bHadPreviousSelection && NewChoice >= PreviousAbilityChoice))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed to select a new ability at time %f. %s"),
			*GetOwner()->GetName(), GetWorld()->GetGameState()->GetServerWorldTimeSeconds(),
			*FString(NewChoice == -1 ? "No valid choice" : "Higher priority choice already selected"));
		
		if (!bHadPreviousSelection)
		{
			CurrentAbilityChoice = -1;
			//If we failed to select an ability, and we aren't already moving, we can try and select a move.
			if (CurrentMovementChoice != -1)
			{
				SelectMovementChoice();
			}
			GetWorld()->GetTimerManager().SetTimer(AbilityChoiceRetryHandle, this, &UNPCAbilityComponent::SelectAbilityChoice, AbilityChoiceRetryTime, false);
		}
	}
	else
	{
		PositionForChosenAbility();
	}
}

void UNPCAbilityComponent::PositionForChosenAbility()
{
	//TODO: Get an FNPCMovementChoice from the ability. Ignore any conditions.
	//For now we just move straight to executing the ability, normally that would come from the callback from move finished (or failed).
	
	if (true /*TODO: Ability doesn't require specific movement*/)
	{
		//If the ability can't be cast while moving, abort our current move and wait on the CMC to not be moving.
		if (!AbilityPriority[CurrentAbilityChoice].AbilityInstance->IsCastableWhileMoving())
		{
			if (CurrentMovementChoice != -1)
			{
				AbortCurrentMovementChoice();
			}
			if (!MovementComponentRef->IsMoving())
			{
				ExecuteAbilityChoice();
			}
		}
		//If the ability can be cast while moving, we can just go right into starting the cast.
		else
		{
			ExecuteAbilityChoice();
		}
	}
	//TODO: Ability does require specific movement.
	else
	{
		if (CurrentMovementChoice != -1)
		{
			AbortCurrentMovementChoice();
		}
		bMovingFromAbilityChoice = true;
		//TODO: Move for ability.
	}
}

void UNPCAbilityComponent::ExecuteAbilityChoice()
{
	//Use the ability here.
	UseAbility(AbilityPriority[CurrentAbilityChoice].AbilityClass);
	
	if (IsCasting())
	{
		//TODO: Do we want to trigger movement here? Will we already be moving?
	}
	//If we aren't in a casting state (either the ability was instant or it failed), then we set a timer to select the next ability.
	//Otherwise, our cast state callback will trigger this.
	else
	{
		GetWorld()->GetTimerManager().SetTimer(AbilityChoiceRetryHandle, this, &UNPCAbilityComponent::SelectAbilityChoice, AbilityChoiceRetryTime, false);
		//If we weren't already moving, we can move here while waiting to select an ability choice.
		if (CurrentMovementChoice == -1)
		{
			SelectMovementChoice();
		}
	}
}

void UNPCAbilityComponent::SelectMovementChoice()
{
	if (!ShouldMoveFromPriorityList())
	{
		return;
	}
	CurrentMovementChoice = -1;
	for (int i = 0; i < MovementPriority.Num(); i++)
	{
		if (MovementPriority[i].AreConditionsMet())
		{
			CurrentMovementChoice = i;
			UE_LOG(LogTemp, Warning, TEXT("%s: Selected movement choice index %i at time %f"),
				*GetOwner()->GetName(), i, GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
			break;
		}
	}
	if (CurrentMovementChoice == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Didn't find any movement choices that were valid at time %f"), *GetOwner()->GetName(), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
		GetWorld()->GetTimerManager().SetTimer(MovementChoiceRetryHandle, this, &UNPCAbilityComponent::SelectMovementChoice, MovementChoiceRetryTime, false);
	}
	else
	{
		const bool bSuccessfulMovement = MovementPriority[CurrentMovementChoice].ExecuteMovementChoice(AIController);
		if (!bSuccessfulMovement)
		{
			GetWorld()->GetTimerManager().SetTimer(MovementChoiceRetryHandle, this, &UNPCAbilityComponent::SelectMovementChoice, MovementChoiceRetryTime, false);
		}
	}
}

void UNPCAbilityComponent::OnAbilityChoiceAvailable(const int32 Priority)
{
	if (CombatBehavior != ENPCCombatBehavior::Combat)
	{
		return;
	}
	//TODO: Interrupting lower prio task.
	UE_LOG(LogTemp, Warning, TEXT("High priority ability choice became available! %s"), *AbilityPriority[Priority].AbilityClass->GetName());
}

void UNPCAbilityComponent::AbortCurrentAbilityChoice()
{
	if (IsCasting())
	{
		CancelCurrentCast();
	}
	if (bMovingFromAbilityChoice)
	{
		if (IsValid(AIController))
		{
			AIController->StopMovement();
		}
		bMovingFromAbilityChoice = false;
	}
	CurrentAbilityChoice = -1;
}

bool UNPCAbilityComponent::ShouldMoveFromPriorityList() const
{
	return !bMovingFromAbilityChoice && (!IsCasting() || GetCurrentCast()->IsCastableWhileMoving());
}

void UNPCAbilityComponent::OnCastChanged(const FCastingState& PreviousState, const FCastingState& NewState)
{
	//Select a new ability choice whenever we finish a cast.
	if (CombatBehavior == ENPCCombatBehavior::Combat && !NewState.bIsCasting)
	{
		SelectAbilityChoice();
	}
}

void UNPCAbilityComponent::LeaveCombatState()
{
	//If we're using a behavior tree for combat, stop running it.
	if (bUseCustomCombatTree)
	{
		if (IsValid(AIController) && IsValid(AIController->GetBrainComponent()))
		{
			AIController->GetBrainComponent()->StopLogic("Leaving combat");
		}
	}
	//If we're using the ability priority list, abort our current cast and pathfinding.
	else
	{
		OnCastStateChanged.RemoveDynamic(this, &UNPCAbilityComponent::OnCastChanged);
		AbortCurrentAbilityChoice(false);
		AbortCurrentMovementChoice(false);
	}
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
	//Cancel any move in progress.
	if (CurrentMoveRequestID.IsValid() && IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, CurrentMoveRequestID);
	}
}

void UNPCAbilityComponent::SetPatrolSubstate(const EPatrolSubstate NewSubstate)
{
	PatrolSubstate = NewSubstate;
	ATargetPoint* LastReachedPoint = PatrolPath.IsValidIndex(LastPatrolIndex) ? PatrolPath[LastPatrolIndex].Point : nullptr;
	OnPatrolStateChanged.Broadcast(PatrolSubstate, LastReachedPoint);
}

void UNPCAbilityComponent::MoveToNextPatrolPoint()
{
	//If we have finished the patrol, nothing more to do.
	if (!PatrolPath.IsValidIndex(NextPatrolIndex))
	{
		FinishPatrol();
		return;
	}
	const ATargetPoint* NextPoint = PatrolPath[NextPatrolIndex].Point;
	//If the next point isn't valid, we essentially want to skip over it.
	if (!IsValid(NextPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI %s had invalid point at index %i, skipping it."), *GetOwner()->GetName(), NextPatrolIndex);
		IncrementPatrolIndex();
		MoveToNextPatrolPoint();
	}
	else
	{
		SetPatrolSubstate(EPatrolSubstate::MovingToPoint);
		if (IsValid(AIController))
		{
			//Request a move to the next patrol location from the AIController.
			//TODO: These parameters might need to be adjusted, I don't know what some of them do.
			FAIMoveRequest MoveReq(PatrolPath[NextPatrolIndex].Point->GetActorLocation());
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
			}
			//If we were already at the patrol point, move on to the next.
			else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
			{
				OnReachedPatrolPoint();
			}
			//If the path following result failed, log an error and move to the next point.
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("NPC %s failed to path to index %i in its patrol. Result: %s. Moving to next point."),
					*GetOwner()->GetName(), NextPatrolIndex, *UEnum::GetDisplayValueAsText(RequestResult.Code).ToString());
				OnReachedPatrolPoint();
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC %s had an invalid AI Controller when trying to patrol. Ending patrol."), *GetOwner()->GetName());
			FinishPatrol();
		}
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
		SetPatrolSubstate(EPatrolSubstate::WaitingAtPoint);
	}
	else
	{
		IncrementPatrolIndex();
		MoveToNextPatrolPoint();
	}
}

void UNPCAbilityComponent::FinishPatrolWait()
{
	IncrementPatrolIndex();
	MoveToNextPatrolPoint();
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
	SetPatrolSubstate(EPatrolSubstate::PatrolFinished);
	//Cancel any move in progress.
	if (CurrentMoveRequestID.IsValid() && IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, CurrentMoveRequestID);
		CurrentMoveRequestID = FAIRequestID::InvalidRequest;
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
		}
		//If we were already at the reset goal, we can immediately move to patrol state.
		else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			bNeedsReset = false;
			UpdateCombatBehavior();
		}
		//If the path following result failed, log an error and move to the next point.
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC %s failed to path to reset goal. Result: %s. Teleporting."),
				*GetOwner()->GetName(), *UEnum::GetDisplayValueAsText(RequestResult.Code).ToString());
			
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
