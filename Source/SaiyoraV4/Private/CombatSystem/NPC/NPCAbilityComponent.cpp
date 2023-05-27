#include "NPCAbilityComponent.h"
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

#pragma region Combat

FCombatPhase UNPCAbilityComponent::GetCombatPhase() const
{
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseTag.MatchesTagExact(CurrentPhase))
		{
			return Phase;
		}
	}
	return FCombatPhase();
}

void UNPCAbilityComponent::EnterPhase(const FGameplayTag PhaseTag)
{
	if (GetOwnerRole() != ROLE_Authority || CurrentPhase.MatchesTagExact(PhaseTag))
	{
		return;
	}
	for (const FCombatPhase& Phase : Phases)
	{
		if (Phase.PhaseTag.MatchesTagExact(PhaseTag))
		{
			CurrentPhase = PhaseTag;
			if (Phase.bHighPriority)
			{
				//TODO: Interrupt current action, determine new action.
				return;
			}
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
			//TODO: This could easily go wrong if anything else modifies max walk speed while the mob is patrolling. Maybe just don't let this happen?
			//TODO: Is there a way to do this through a service?
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
