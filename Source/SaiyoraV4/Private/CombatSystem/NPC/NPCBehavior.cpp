#include "NPC/NPCBehavior.h"

#include "AbilityChoice.h"
#include "AbilityFunctionLibrary.h"
#include "AIController.h"
#include "CombatStatusComponent.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UNPCBehavior::UNPCBehavior(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> DefaultTree(TEXT("/Game/Saiyora/AI/Generic/BT_AITest"));
	BehaviorTree = DefaultTree.Object;
}

void UNPCBehavior::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has NPC Behavior Component."));
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	ThreatHandlerRef = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	MovementComponentRef = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(GetOwner());
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	Super::InitializeComponent();
}

void UNPCBehavior::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	OwnerAsPawn = Cast<APawn>(GetOwner());
	checkf(IsValid(OwnerAsPawn), TEXT("NPC Behavior Component's owner was not a valid pawn."));
	GameStateRef = Cast<ADungeonGameState>(GetWorld()->GetGameState());
	checkf(IsValid(GameStateRef), TEXT("Got an invalid Game State Ref in NPC Behavior Component."));
	
	OwnerAsPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UNPCBehavior::OnControllerChanged);
	GameStateRef->OnDungeonPhaseChanged.AddDynamic(this, &UNPCBehavior::OnDungeonPhaseChanged);
	if (IsValid(DamageHandlerRef))
	{
		DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UNPCBehavior::OnLifeStatusChanged);
	}
	if (IsValid(ThreatHandlerRef))
	{
		ThreatHandlerRef->OnCombatChanged.AddDynamic(this, &UNPCBehavior::OnCombatChanged);
	}
	OnControllerChanged(OwnerAsPawn, nullptr, OwnerAsPawn->GetController());
}

void UNPCBehavior::OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	UpdateCombatStatus();
}

void UNPCBehavior::OnCombatChanged(const bool bInCombat)
{
	UpdateCombatStatus();
}

void UNPCBehavior::UpdateLosAndRangeInfo()
{
	if (CombatStatus == ENPCCombatStatus::Combat && IsValid(ThreatHandlerRef) && IsValid(ThreatHandlerRef->GetCurrentTarget()))
	{
		const ESaiyoraPlane OwnerPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
		const FName TraceProfile = UAbilityFunctionLibrary::GetRelevantTraceProfile(GetOwner(), true, OwnerPlane, EFaction::Enemy);
		FName OriginSocket = NAME_None;
		const USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(GetOwner(), OriginSocket);
		FVector FromLocation = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : GetOwner()->GetActorLocation();
		TArray<AActor*> ToIgnore;
		ToIgnore.Add(GetOwner());
		ToIgnore.Add(ThreatHandlerRef->GetCurrentTarget());
		FHitResult TraceResult;
		const bool bBlockingHit = UKismetSystemLibrary::LineTraceSingleByProfile(GetOwner(), FromLocation, ThreatHandlerRef->GetCurrentTarget()->GetActorLocation(), TraceProfile,
			false, ToIgnore, EDrawDebugTrace::ForDuration, TraceResult, true, FLinearColor::Blue, FLinearColor::Red, 1.0f);
	
		bInLineOfSight = !bBlockingHit;
		Range = (ThreatHandlerRef->GetCurrentTarget()->GetActorLocation() - FromLocation).Length();
	}
	else
	{
		bInLineOfSight = false;
		Range = -1.0f;
	}
	for (UAbilityChoice* Choice : CurrentChoices)
	{
		Choice->UpdateRangeAndLos(Range, bInLineOfSight);
	}
}

void UNPCBehavior::EnterCombatState()
{
	//Set up line of sight and range checking on an interval.
	//Update ability choices that care about line of sight specifically to target (not to friendlies).
}

void UNPCBehavior::MarkResetComplete()
{
	bNeedsReset = false;
	UpdateCombatStatus();
}

void UNPCBehavior::OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController)
{
	AIController = Cast<AAIController>(NewController);
	UpdateCombatStatus();
}

void UNPCBehavior::OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase)
{
	UpdateCombatStatus();
}

void UNPCBehavior::UpdateCombatStatus()
{
	ENPCCombatStatus NewCombatStatus = ENPCCombatStatus::None;
	if (GameStateRef->GetDungeonPhase() == EDungeonPhase::InProgress)
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
			//TODO: Leave combat.
			break;
		case ENPCCombatStatus::Patrolling :
			LeavePatrolState();
			break;
		case ENPCCombatStatus::Resetting :
			//TODO: Leave resetting.
			break;
		default:
			//TODO: Leave none, so maybe set up the right behavior tree?
			AIController->RunBehaviorTree(BehaviorTree);
			AIController->GetBlackboardComponent()->SetValueAsObject("BehaviorComponent", this);
			break;
		}
		CombatStatus = NewCombatStatus;
		switch (CombatStatus)
		{
		case ENPCCombatStatus::Combat :
			//TODO: Enter combat.
			break;
		case ENPCCombatStatus::Patrolling :
			EnterPatrolState();
			break;
		case ENPCCombatStatus::Resetting :
			//TODO: Enter resetting.
			break;
		default:
			break;
		}
		AIController->GetBlackboardComponent()->SetValueAsEnum("CombatStatus", uint8(CombatStatus));
	}
}

void UNPCBehavior::ReachedPatrolPoint(const FVector& Location)
{
	NextPatrolIndex += 1;
	OnPatrolLocationReached.Broadcast(Location);
}

void UNPCBehavior::SetNextPatrolPoint(UBlackboardComponent* Blackboard)
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

void UNPCBehavior::EnterPatrolState()
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
			//TODO: This could easily go wrong if anything else modifies max walk speed while the mob is patrolling. Maybe just don't let this happen?
			//TODO: Is there a way to do this through a service?
			DefaultMaxWalkSpeed = MovementComponentRef->GetDefaultMaxWalkSpeed();
			MovementComponentRef->MaxWalkSpeed = DefaultMaxWalkSpeed * PatrolMoveSpeedModifier;
		}
	}
}

void UNPCBehavior::LeavePatrolState()
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
}

FCombatPhase UNPCBehavior::GetCurrentPhase() const
{
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseIndex == CurrentPhaseIndex)
		{
			return Phase;
		}
	}
	return FCombatPhase();
}

void UNPCBehavior::EnterPhase(const int32 PhaseIndex)
{
	if (PhaseIndex == CurrentPhaseIndex)
	{
		return;
	}
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseIndex == PhaseIndex)
		{
			NewPhaseIndex = PhaseIndex;
			bReadyForPhaseChange = true;
			if (Phase.bHighPriority)
			{
				InterruptCurrentAction();
				StartNewAction();
			}
		}
	}
}

void UNPCBehavior::InterruptCurrentAction()
{
	//TODO: Interrupt cast if one is active.
	//Unsubscribe from ability finished casting?
	//Should we stop movement? BT can probably handle that part of things?
}

void UNPCBehavior::StartNewAction()
{
	if (bReadyForPhaseChange)
	{
		for (UAbilityChoice* Choice : CurrentChoices)
		{
			Choice->CleanUp();
		}
		CurrentChoices.Empty();
		CurrentPhaseIndex = NewPhaseIndex;
		NewPhaseIndex = -1;
		for (const FCombatPhase& Phase : Phases)
		{
			if (Phase.PhaseIndex == CurrentPhaseIndex)
			{
				for (const TSubclassOf<UAbilityChoice> Choice : Phase.AbilityChoices)
				{
					UAbilityChoice* NewChoice = NewObject<UAbilityChoice>(this, Choice);
					if (IsValid(NewChoice))
					{
						CurrentChoices.Add(NewChoice);
						NewChoice->OnScoreChanged.BindDynamic(this, &UNPCBehavior::OnChoiceScoreChanged);
						NewChoice->InitializeChoice(this);
					}
				}
				break;
			}
		}
	}
	//Set blackboard keys for ability class, target, line of sight, distance?
	if (IsValid(CurrentChoices[0]))
	{
		ActionInProgress = CurrentChoices[0];
		AIController->GetBlackboardComponent()->SetValueAsClass("AbilityToUse", ActionInProgress->GetAbilityClass());
		AIController->GetBlackboardComponent()->SetValueAsBool("CanUseAbilityWhileMoving", ActionInProgress->CanUseWhileMoving());
		/*AIController->GetBlackboardComponent()->SetValueAsBool("AbilityNeedsTarget", ActionInProgress->RequiresTarget());
		if (ActionInProgress->RequiresTarget())
		{
			AIController->GetBlackboardComponent()->SetValueAsObject("Target", ActionInProgress->GetTarget());
			AIController->GetBlackboardComponent()->SetValueAsFloat("AbilityRange", ActionInProgress->GetAbilityRange());
			AIController->GetBlackboardComponent()->SetValueAsBool("NeedsLineOfSight", ActionInProgress->RequiresLineOfSight());
			if (ActionInProgress->RequiresLineOfSight())
			{
				AIController->GetBlackboardComponent()->SetValueAsEnum("LineOfSightPlane", (uint8)ActionInProgress->GetLineOfSightPlane());
			}
		}*/
	}
	else
	{
		//TODO: No valid choices?
	}
}

void UNPCBehavior::OnChoiceScoreChanged(UAbilityChoice* Choice, const float NewScore)
{
	for (int32 i = 0; i < CurrentChoices.Num(); i++)
	{
		if (CurrentChoices[i] == Choice)
		{
			//If score is now higher than the previous item's score:
			if (CurrentChoices.IsValidIndex(i - 1) && CurrentChoices[i - 1]->GetCurrentScore() < NewScore)
			{
				do
				{
					CurrentChoices.Swap(i, i - 1);
					i--;
				}
				while (CurrentChoices.IsValidIndex(i - 1) && CurrentChoices[i - 1]->GetCurrentScore() < NewScore);
			}
			//else If score is now lower than next item's score.
			else if (CurrentChoices.IsValidIndex(i + 1) && CurrentChoices[i + 1]->GetCurrentScore() > NewScore)
			{
				do
				{
					CurrentChoices.Swap(i, i + 1);
					i++;
				}
				while (CurrentChoices.IsValidIndex(i + 1) && CurrentChoices[i + 1]->GetCurrentScore() > NewScore);
			}
			break;
		}
	}
}
