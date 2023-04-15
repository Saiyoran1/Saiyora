#include "NPC/NPCBehavior.h"
#include "AIController.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "BehaviorTree/BlackboardComponent.h"

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

void UNPCBehavior::OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController)
{
	AIController = Cast<AAIController>(NewController);
	UpdateCombatStatus();
}

void UNPCBehavior::OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase)
{
	UpdateCombatStatus();
}

void UNPCBehavior::StopBehavior()
{
	
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
			//TODO: Leave patrolling.
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

void UNPCBehavior::ReachedPatrolPoint()
{
	NextPatrolIndex += 1;
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
			DefaultMaxWalkSpeed = MovementComponentRef->GetDefaultMaxWalkSpeed();
			MovementComponentRef->MaxWalkSpeed = DefaultMaxWalkSpeed * PatrolMoveSpeedModifier;
		}
	}
}



