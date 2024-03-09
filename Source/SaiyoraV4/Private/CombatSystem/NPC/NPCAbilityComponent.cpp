#include "NPCAbilityComponent.h"

#include "CombatLink.h"
#include "DamageHandler.h"
#include "NPCAbility.h"
#include "PatrolRoute.h"
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
	PrimaryComponentTick.bCanEverTick = false;
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
	DungeonGameStateRef = Cast<ADungeonGameState>(GetGameStateRef());
	checkf(IsValid(DungeonGameStateRef), TEXT("Got an invalid Game State Ref in NPC Ability Component."));
	
	OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::UpdateAbilityTokensOnCastStateChanged);

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
	
	if (IsValid(PatrolRoute))
	{
		PatrolRoute->GetPatrolRoute(PatrolPath);
	}
}

void UNPCAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPCAbilityComponent, CombatBehavior);
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
				break;
			default :
				return;
			}
		}
	}
}

#pragma endregion 
#pragma region Combat

void UNPCAbilityComponent::EnterCombatState()
{
	//Run the combat behavior tree for this actor.
	if (!IsValid(CombatTree) || !IsValid(AIController))
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %s failed to run a custom combat tree."), *GetOwner()->GetName());
		return;
	}
	
	if (!AIController->RunBehaviorTree(CombatTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %s failed to run a custom combat tree."), *GetOwner()->GetName());
	}
}


void UNPCAbilityComponent::LeaveCombatState()
{
	//If we're using a behavior tree for combat, stop running it.
	if (IsValid(AIController) && IsValid(AIController->GetBrainComponent()))
	{
		AIController->GetBrainComponent()->StopLogic("Leaving combat");
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
	
	SetPatrolSubstate(EPatrolSubstate::MovingToPoint);
	if (IsValid(AIController))
	{
		//Request a move to the next patrol location from the AIController.
		//TODO: These parameters might need to be adjusted, I don't know what some of them do.
		FAIMoveRequest MoveReq(PatrolPath[NextPatrolIndex].Location);
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
	SetPatrolSubstate(EPatrolSubstate::PatrolFinished);
	//Cancel any move in progress.
	if (CurrentMoveRequestID.IsValid() && IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, CurrentMoveRequestID);
		CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	}
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
	if (CombatBehavior == ENPCCombatBehavior::Patrolling && PatrolSubstate != EPatrolSubstate::PatrolFinished)
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
#pragma region Tokens

void UNPCAbilityComponent::UpdateAbilityTokensOnCastStateChanged(const FCastingState& PreviousState, const FCastingState& NewState)
{
	//If this was the start of a cast, and the ability being used is an NPCAbility that requires tokens,
	//then we should request a token.
	if (!PreviousState.bIsCasting && NewState.bIsCasting)
	{
		UNPCAbility* NPCAbility = Cast<UNPCAbility>(NewState.CurrentCast);
		if (IsValid(NPCAbility) && NPCAbility->UsesTokens())
		{
			const bool bGotToken = GetGameStateRef()->RequestTokenForAbility(NPCAbility);
			if (!bGotToken)
			{
				UE_LOG(LogTemp, Error, TEXT("Ability %s used even though no token was available!"), *NPCAbility->GetName());
			}
		}
	}
	//If this was the end of a cast, and the ability being cast was an NPCAbility that requires tokens,
	//then we should return the token.
	else if (PreviousState.bIsCasting && !NewState.bIsCasting)
	{
		UNPCAbility* NPCAbility = Cast<UNPCAbility>(PreviousState.CurrentCast);
		if (IsValid(NPCAbility) && NPCAbility->UsesTokens())
		{
			GetGameStateRef()->ReturnTokenForAbility(NPCAbility);
		}
	}
}

#pragma endregion
#pragma region Test Combat Choices

void UNPCAbilityComponent::InitCombatChoices()
{
	for (int i = 0; i < CombatPriority.Num(); i++)
	{
		CombatPriority[i].Init(this, i);
	}
	TrySelectNewChoice();
}

void UNPCAbilityComponent::TrySelectNewChoice()
{
	//If we are in the actual ability usage, and this choice shouldn't be aborted during the cast, don't pick a new choice.
	if (CurrentCombatChoiceIdx != -1 && CombatChoiceStatus == ENPCCombatChoiceStatus::Casting
		&& !CombatPriority[CurrentCombatChoiceIdx].CanAbortDuringCast())
	{
		return;
	}
	//Otherwise, we need to check for the highest priority choice. We are only looking for choices below our current index (unless the index is -1, which means we have no choice).
	const int LastIdxToCheck = CurrentCombatChoiceIdx == -1 ? CombatPriority.Num() - 1 : CurrentCombatChoiceIdx - 1;
	int NewIdx = -1;
	for (int i = 0; i < LastIdxToCheck; i++)
	{
		if (CombatPriority[i].IsChoiceValid()
			&& (CombatPriority[i].IsHighPriority() || CurrentCombatChoiceIdx == -1))
		{
			NewIdx = i;
			break;
		}
	}

	if (NewIdx == -1)
	{
		//If we failed to find a higher priority choice, we return here.
		//Going to try NOT doing a retry timer, as the choices should be updating the component when they're available anyway.
		return;
	}

	if (CurrentCombatChoiceIdx != -1)
	{
		AbortCurrentChoice();
	}

	CurrentCombatChoiceIdx = NewIdx;
	StartExecuteChoice();
}

void UNPCAbilityComponent::StartExecuteChoice()
{
	//TODO: Check if we need to move. If so, start a move. If not, go straight to casting. If cast is instant, return to TrySelectNewChoice.
}

void UNPCAbilityComponent::AbortCurrentChoice()
{
	if (CurrentCombatChoiceIdx == -1)
	{
		return;
	}
	CombatPriority[CurrentCombatChoiceIdx].Abort();
	CurrentCombatChoiceIdx = -1;
	CombatChoiceStatus = ENPCCombatChoiceStatus::None;
}

void UNPCAbilityComponent::OnChoiceBecameValid(const int ChoiceIdx)
{
	
}

#pragma endregion 