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
#include "EnvironmentQuery/EnvQuery.h"
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
		//If we switch behaviors, we should abort any active queries or moves in progress.
		AbortActiveQuery();
		AbortActiveMove();
		
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

void UNPCAbilityComponent::AbortActiveQuery()
{
	if (QueryID != INDEX_NONE && IsValid(UEnvQueryManager::GetCurrent(GetWorld())))
	{
		UEnvQueryManager::GetCurrent(GetWorld())->AbortQuery(QueryID);
		QueryID = INDEX_NONE;
	}
}

void UNPCAbilityComponent::AbortActiveMove()
{
	if (CurrentMoveRequestID.IsValid() && IsValid(AIController) && IsValid(AIController->GetPathFollowingComponent()))
	{
		AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, CurrentMoveRequestID);
		CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	}
}

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
	InitCombatChoices();
	if (CombatPriority.Num() > 0)
	{
		TrySelectNewChoice();
	}
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
	UE_LOG(LogTemp, Warning, TEXT("Trying to select choice."));
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
	
	UE_LOG(LogTemp, Warning, TEXT("Choice made: %i, %s."), ChoiceIdx, *CombatPriority[ChoiceIdx].DEBUG_GetDisplayName());

	const UCombatAbility* AbilityInstance = FindActiveAbility(CombatPriority[ChoiceIdx].GetAbilityClass());
	if (!IsValid(AbilityInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("Choice %i (%s) could not get a valid ability instance."), ChoiceIdx, *CombatPriority[ChoiceIdx].DEBUG_GetDisplayName());
		//If we failed to find a valid choice, we will try again in a bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
		return;
	}
	if (!AbilityInstance->IsCastableWhileMoving())
	{
		//TODO: Stop moving
	}
	const FAbilityEvent AbilityEvent = UseAbility(CombatPriority[ChoiceIdx].GetAbilityClass());
	UE_LOG(LogTemp, Warning, TEXT("Cast ability %s, result was %s."), *AbilityInstance->GetAbilityName().ToString(), *UEnum::GetDisplayValueAsText(AbilityEvent.ActionTaken).ToString());
	if (AbilityEvent.ActionTaken == ECastAction::Success && AbilityInstance->GetCastType() == EAbilityCastType::Channel)
	{
		//Bind to the cast end delegate so that we can select a new choice when this ability ends.
		OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::EndChoiceOnCastStateChanged);
	}
	else
	{
		//After casting this ability, we'll select another choice in a little bit.
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
	}
}

void UNPCAbilityComponent::EndChoiceOnCastStateChanged(const FCastingState& Previous, const FCastingState& New)
{
	if (!New.bIsCasting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cast ended (%s), re-selecting in %s seconds."), *Previous.CurrentCast->GetAbilityName().ToString(), *FString::SanitizeFloat(ChoiceRetryDelay));
		OnCastStateChanged.RemoveDynamic(this, &UNPCAbilityComponent::EndChoiceOnCastStateChanged);
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay);
	}
}

void UNPCAbilityComponent::RunQuery(const UEnvQuery* Query, const TArray<FInstancedStruct>& QueryParams)
{
	if (IsValid(Query))
	{
		FEnvQueryRequest Request (Query);
		for (const FInstancedStruct& InstancedParam : QueryParams)
		{
			const FNPCQueryParam* Param = InstancedParam.GetPtr<FNPCQueryParam>();
			if (!Param)
			{
				continue;
			}
			Param->AddParam(Request);
		}
		QueryID = Request.Execute(EEnvQueryRunMode::SingleResult, this, &UNPCAbilityComponent::OnQueryFinished);
		if (QueryID == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Query failed to run, re-selecting in %s seconds."), *FString::SanitizeFloat(ChoiceRetryDelay));
			GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Running query %i"), QueryID);
		}
	}
	else
	{
		QueryID = INDEX_NONE;
		UE_LOG(LogTemp, Warning, TEXT("Invalid query, re-selecting choice in %s seconds."), *FString::SanitizeFloat(ChoiceRetryDelay));
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay, false);
	}
}

void UNPCAbilityComponent::OnQueryFinished(TSharedPtr<FEnvQueryResult> QueryResult)
{
	if (!QueryResult.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Query %i finished but result wasn't valid. Re-selecting in %s seconds."), QueryID, *FString::SanitizeFloat(ChoiceRetryDelay));
		QueryID = INDEX_NONE;
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay, false);
		return;
	}
	const FEnvQueryResult* Result = QueryResult.Get();
	if (Result->QueryID != QueryID)
	{
		UE_LOG(LogTemp, Warning, TEXT("Query %i finished but current query ID was %i, ignoring."), Result->QueryID, QueryID);
		return;
	}
	if (!Result->IsFinished() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Query %i finished but was unsuccessful. Re-selecting in %s seconds."), QueryID, *FString::SanitizeFloat(ChoiceRetryDelay));
		QueryID = INDEX_NONE;
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay, false);
		return;
	}

	bHasValidMoveLocation = true;
	CurrentMoveLocation = Result->GetItemAsLocation(0);
	QueryID = INDEX_NONE;

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
	const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveReq);
	if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		CurrentMoveRequestID = RequestResult.MoveId;
		UE_LOG(LogTemp, Warning, TEXT("Move request successful, ID %i"), CurrentMoveRequestID.GetID());
	}
	//If we were already at the desired location, go ahead and use the ability.
	else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		UE_LOG(LogTemp, Warning, TEXT("Move request failed because already at goal!"));
	}
	//If the path following result failed, try to do something else.
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Move request failed, re-selecting in %s seconds"), *FString::SanitizeFloat(ChoiceRetryDelay));
		GetWorld()->GetTimerManager().SetTimer(ChoiceRetryHandle, this, &UNPCAbilityComponent::TrySelectNewChoice, ChoiceRetryDelay, false);
	}
}

void UNPCAbilityComponent::LeaveCombatState()
{
	//Stop the choice selection retry timer.
	if (GetWorld()->GetTimerManager().IsTimerActive(ChoiceRetryHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(ChoiceRetryHandle);
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
	//Cancel any move in progress.
	AbortActiveMove();
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