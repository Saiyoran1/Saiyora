#include "NPCAbilityComponent.h"

#include "AbilityCondition.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
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
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
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

void UNPCAbilityComponent::UpdateCombatStatus()
{
	ENPCCombatStatus NewCombatStatus = ENPCCombatStatus::None;
	if (DungeonGameStateRef->GetDungeonPhase() == EDungeonPhase::InProgress)
	{
		if (IsValid(AIController))
		{
			if (!IsValid(DamageHandlerRef) || DamageHandlerRef->GetLifeStatus() == ELifeStatus::Alive)
			{
				if (IsValid(ThreatHandlerRef) && ThreatHandlerRef->IsInCombat())
				{
					NewCombatStatus = ENPCCombatStatus::Combat;
				}
				else
				{
					NewCombatStatus = bNeedsReset ? ENPCCombatStatus::Resetting : ENPCCombatStatus::Patrolling;
				}
			}
		}
	}
	if (NewCombatStatus != CombatStatus)
	{
		switch(CombatStatus)
		{
		case ENPCCombatStatus::Combat :
			LeaveCombatState();
			break;
		case ENPCCombatStatus::Patrolling :
			LeavePatrolState();
			break;
		case ENPCCombatStatus::Resetting :
			LeaveResetState();
			break;
		default:
			SetupBehavior();
			break;
		}
		CombatStatus = NewCombatStatus;
		switch (CombatStatus)
		{
		case ENPCCombatStatus::Combat :
			EnterCombatState();
			break;
		case ENPCCombatStatus::Patrolling :
			EnterPatrolState();
			break;
		case ENPCCombatStatus::Resetting :
			EnterResetState();
			break;
		default:
			break;
		}
		if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
		{
			AIController->GetBlackboardComponent()->SetValueAsEnum("CombatStatus", uint8(CombatStatus));
		}
	}
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
		for (const TSubclassOf<UAbilityCondition> ConditionClass : Choice.AbilityConditions)
		{
			if (IsValid(ConditionClass))
			{
				const UAbilityCondition* DefaultCondition = ConditionClass.GetDefaultObject();
				if (!IsValid(DefaultCondition))
				{
					bConditionFailed = true;
					break;
				}
				if (!DefaultCondition->IsConditionMet(GetOwner(), Choice.AbilityClass))
				{
					bConditionFailed = true;
					break;
				}
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
		if (CombatStatus == ENPCCombatStatus::Combat)
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
}

void UNPCAbilityComponent::LeavePatrolState()
{
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
	ResetGoal = GetOwner()->GetActorLocation();
	bNeedsReset = true;
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
	while (!Patrol.IsValidIndex(NextPatrolIndex) || !IsValid(Patrol[NextPatrolIndex].Point))
	{
		if (NextPatrolIndex > Patrol.Num() - 1)
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
	Blackboard->SetValueAsObject("PatrolGoal", Patrol[NextPatrolIndex].Point);
	Blackboard->SetValueAsFloat("WaitTime", Patrol[NextPatrolIndex].WaitTime);
}

#pragma endregion
#pragma region Resetting

void UNPCAbilityComponent::MarkResetComplete()
{
	if (CombatStatus != ENPCCombatStatus::Resetting)
	{
		return;
	}
	bNeedsReset = false;
	UpdateCombatStatus();
}

void UNPCAbilityComponent::EnterResetState()
{
	if (IsValid(AIController) && IsValid(AIController->GetBlackboardComponent()))
	{
		AIController->GetBlackboardComponent()->SetValueAsVector("ResetGoal", ResetGoal);
	}
	//Reset health
	//Prevent damage
	//Prevent threat
	//Disable hitboxes
}

void UNPCAbilityComponent::LeaveResetState()
{
	//Re-enable damage
	//Re-enable threat
	//Re-enable hitboxes
}

#pragma endregion 
