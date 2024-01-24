#include "NPCAbilityComponent.h"
#include "AbilityCondition.h"
#include "AssetRegistryTelemetry.h"
#include "DamageHandler.h"
#include "NavigationSystem.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "UnrealNetwork.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

UNPCAbilityComponent::UNPCAbilityComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> DefaultTree(TEXT("/Game/Saiyora/AI/Generic/BT_AITest"));
	//Set default value for general behavior tree. If hard-coding asset path is a problem, just derive a blueprint class to use as the "master" component.
	BehaviorTree = DefaultTree.Object;
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

	TSet<TSubclassOf<UCombatAbility>> PhaseAbilities;
	for (const FCombatPhase& Phase : Phases)
	{
		for (const FAbilityChoice& Choice :  Phase.AbilityChoices)
		{
			if (IsValid(Choice.AbilityClass))
			{
				PhaseAbilities.Add(Choice.AbilityClass);	
			}
		}
	}
	for (const TSubclassOf<UCombatAbility> AbilityClass : PhaseAbilities)
	{
		AddNewAbility(AbilityClass);
	}

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
}

void UNPCAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPCAbilityComponent, CombatBehavior);
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
			SetupBehavior();
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
		if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
		{
			AIController->GetBlackboardComponent()->SetValueAsEnum("CombatStatus", uint8(CombatBehavior));
		}
		OnCombatBehaviorChanged.Broadcast(PreviousBehavior, CombatBehavior);
	}
}

void UNPCAbilityComponent::OnRep_CombatBehavior(const ENPCCombatBehavior Previous)
{
	OnCombatBehaviorChanged.Broadcast(Previous, CombatBehavior);
}

void UNPCAbilityComponent::SetupBehavior()
{
	if (bInitialized)
	{
		return;
	}
	if (!IsValid(AIController))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid AI controller ref in NPCAbilityComponent."));
		return;
	}
	AIController->RunBehaviorTree(BehaviorTree);
	if (!IsValid(AIController->GetBlackboardComponent()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Blackboard Component ref in NPCAbilityComponent."));
		return;
	}
	AIController->GetBlackboardComponent()->SetValueAsObject("BehaviorComponent", this);
	if (IsValid(ThreatHandlerRef))
	{
		ThreatHandlerRef->OnTargetChanged.AddDynamic(this, &UNPCAbilityComponent::OnTargetChanged);
		OnTargetChanged(nullptr, ThreatHandlerRef->GetCurrentTarget());
	}
	bInitialized = true;
}

void UNPCAbilityComponent::OnTargetChanged(AActor* PreviousTarget, AActor* NewTarget)
{
	if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
	{
		AIController->GetBlackboardComponent()->SetValueAsObject("Target", NewTarget);
	}
}

#pragma region Combat

FCombatPhase UNPCAbilityComponent::GetCombatPhase() const
{
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseTag.MatchesTagExact(CurrentPhaseTag))
		{
			return Phase;
		}
	}
	return FCombatPhase();
}

void UNPCAbilityComponent::EnterPhase(const FGameplayTag PhaseTag)
{
	if (GetOwnerRole() != ROLE_Authority || CurrentPhaseTag.MatchesTagExact(PhaseTag))
	{
		return;
	}
	//TODO: Add some protection for not having a default phase?
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseTag.MatchesTagExact(PhaseTag))
		{
			CurrentPhaseTag = PhaseTag;
			if (Phase.bHighPriority)
			{
				InterruptCurrentAction();
				DetermineNewAction();
			}
			//If not high priority, next DetermineNewAction will simply just be looking at the new phase list.
		}
	}
}

void UNPCAbilityComponent::EnterCombatState()
{
	EnterPhase(DefaultPhase);
	if (!GetCombatPhase().bHighPriority)
	{
		//Manually determine a new action, since only high priority phases will instantly determine one.
		DetermineNewAction();
	}
}

void UNPCAbilityComponent::LeaveCombatState()
{
	InterruptCurrentAction();
	CurrentPhaseTag = FGameplayTag::EmptyTag;
	if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
	{
		AIController->GetBlackboardComponent()->SetValueAsBool("CombatMoving", false);
	}
}

void UNPCAbilityComponent::DetermineNewAction()
{
	const FCombatPhase CombatPhase = GetCombatPhase();
	TSubclassOf<UCombatAbility> ChosenAbility = nullptr;
	for (const FAbilityChoice& Choice : CombatPhase.AbilityChoices)
	{
		if (!IsValid(Choice.AbilityClass))
		{
			continue;
		}
		const UCombatAbility* Ability = FindActiveAbility(Choice.AbilityClass);
		ECastFailReason FailReason = ECastFailReason::None;
		if (!CanUseAbility(Ability, FailReason))
		{
			continue;
		}
		bool bConditionFailed = false;
		for (const FInstancedStruct& Condition : Choice.AbilityConditions)
		{
			if (const FNPCAbilityCondition* AbilityCondition = Condition.GetPtr<FNPCAbilityCondition>())
			{
				if (!AbilityCondition->CheckCondition(GetOwner(), Choice.AbilityClass))
				{
					bConditionFailed = true;
					break;
				}
			}
			else
			{
				bConditionFailed = true;
				break;
			}
		}
		if (!bConditionFailed)
		{
			ChosenAbility = Choice.AbilityClass;
			break;
		}
	}
	if (IsValid(ChosenAbility))
	{
		const FAbilityEvent AbilityEvent = UseAbility(ChosenAbility);
		if (AbilityEvent.ActionTaken == ECastAction::Fail)
		{
			if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
			{
				AIController->GetBlackboardComponent()->SetValueAsBool("CombatMoving", true);
			}
			GetWorld()->GetTimerManager().SetTimer(ActionRetryHandle, this, &UNPCAbilityComponent::DetermineNewAction, ActionRetryFrequency);
		}
		else
		{
			if (IsCasting())
			{
				OnCastStateChanged.AddDynamic(this, &UNPCAbilityComponent::OnCastEnded);
				if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
				{
					AIController->GetBlackboardComponent()->SetValueAsBool("CombatMoving", GetCurrentCast()->IsCastableWhileMoving());
				}
			}
			else
			{
				if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
				{
					AIController->GetBlackboardComponent()->SetValueAsBool("CombatMoving", true);
				}
				DetermineNewAction();
			}
		}
	}
	else
	{
		if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
		{
			AIController->GetBlackboardComponent()->SetValueAsBool("CombatMoving", true);
		}
		GetWorld()->GetTimerManager().SetTimer(ActionRetryHandle, this, &UNPCAbilityComponent::DetermineNewAction, ActionRetryFrequency);
	}
}

void UNPCAbilityComponent::InterruptCurrentAction()
{
	OnCastStateChanged.RemoveDynamic(this, &UNPCAbilityComponent::OnCastEnded);
	GetWorld()->GetTimerManager().ClearTimer(ActionRetryHandle);
	if (IsCasting())
	{
		CancelCurrentCast();
	}
}

void UNPCAbilityComponent::OnCastEnded(const FCastingState& PreviousState, const FCastingState& NewState)
{
	if (!NewState.bIsCasting)
	{
		OnCastStateChanged.RemoveDynamic(this, &UNPCAbilityComponent::OnCastEnded);
		if (CombatBehavior == ENPCCombatBehavior::Combat)
		{
			DetermineNewAction();
		}
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

void UNPCAbilityComponent::SetPatrolSubstate(const EPatrolSubstate NewSubstate)
{
	PatrolSubstate = NewSubstate;
	ATargetPoint* LastReachedPoint = PatrolPath.IsValidIndex(LastPatrolIndex) ? PatrolPath[LastPatrolIndex].Point : nullptr;
	OnPatrolStateChanged.Broadcast(PatrolSubstate, LastReachedPoint);
}

void UNPCAbilityComponent::MoveToNextPatrolPoint()
{
	UE_LOG(LogTemp, Warning, TEXT("AI %s moving to patrol index %i!"), *GetOwner()->GetName(), NextPatrolIndex);
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
		//TODO: Actually MoveTo the location of the point.
		OnReachedPatrolPoint();
	}
}

void UNPCAbilityComponent::OnReachedPatrolPoint()
{
	UE_LOG(LogTemp, Warning, TEXT("AI %s reached patrol point %i."), *GetOwner()->GetName(), NextPatrolIndex);
	LastPatrolIndex = NextPatrolIndex;
	if (PatrolPath[NextPatrolIndex].WaitTime > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI %s starting a wait for %f seconds."), *GetOwner()->GetName(), PatrolPath[NextPatrolIndex].WaitTime);
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
	UE_LOG(LogTemp, Warning, TEXT("AI %s completed wait time."), *GetOwner()->GetName());
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
	UE_LOG(LogTemp, Warning, TEXT("AI %s finished their patrol."), *GetOwner()->GetName());
	SetPatrolSubstate(EPatrolSubstate::PatrolFinished);
	//TODO: Cancel any nav moving that is happening?
}

void UNPCAbilityComponent::ReachedPatrolPoint(const FVector& Location)
{
	NextPatrolIndex += 1;
	OnPatrolLocationReached.Broadcast(Location);
}

void UNPCAbilityComponent::SetNextPatrolPoint(UBlackboardComponent* Blackboard)
{
	if (!IsValid(Blackboard))
	{
		return;
	}
	const int32 StartingIndex = NextPatrolIndex;
	while (!PatrolPath.IsValidIndex(NextPatrolIndex) || !IsValid(PatrolPath[NextPatrolIndex].Point))
	{
		if (NextPatrolIndex > PatrolPath.Num() - 1)
		{
			if (bLoopPatrol)
			{
				NextPatrolIndex = 0;
			}
			else
			{
				Blackboard->SetValueAsBool("FinishedPatrolling", true);
				return;
			}
		}
		else
		{
			NextPatrolIndex += 1;
		}
		if (NextPatrolIndex == StartingIndex)
		{
			Blackboard->SetValueAsBool("FinishedPatrolling", true);
			return;
		}
	}
	Blackboard->SetValueAsObject("PatrolGoal", PatrolPath[NextPatrolIndex].Point);
	Blackboard->SetValueAsFloat("WaitTime", PatrolPath[NextPatrolIndex].WaitTime);
}

#pragma endregion
#pragma region Resetting

void UNPCAbilityComponent::MarkResetComplete()
{
	if (CombatBehavior != ENPCCombatBehavior::Resetting)
	{
		return;
	}
	bNeedsReset = false;
	UpdateCombatBehavior();
}

void UNPCAbilityComponent::EnterResetState()
{
	if (FVector::DistSquared(GetOwner()->GetActorLocation(), ResetGoal) > FMath::Square(ResetTeleportDistance))
	{
		GetOwner()->SetActorLocation(ResetGoal);
		bNeedsReset = false;
		UpdateCombatBehavior();
	}
	else
	{
		//TODO: Actually move to the reset goal.
		UNavigationSystemV1;
	}
}

void UNPCAbilityComponent::LeaveResetState()
{
	//If I need any specific behavior not handled by other components it can go here.
}

#pragma endregion 
