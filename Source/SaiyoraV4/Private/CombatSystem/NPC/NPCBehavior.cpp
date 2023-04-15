#include "NPC/NPCBehavior.h"
#include "AIController.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "BehaviorTree/BlackboardComponent.h"

UNPCBehavior::UNPCBehavior()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
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
	
}

void UNPCBehavior::OnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{

}

void UNPCBehavior::OnCombatChanged(const bool bInCombat)
{

}

void UNPCBehavior::OnControllerChanged(APawn* PossessedPawn, AController* OldController, AController* NewController)
{
	AIController = Cast<AAIController>(NewController);
	if (!IsValid(AIController))
	{
		StopBehavior();
		return;
	}
	if (CombatStatus == ENPCCombatStatus::None)
	{
		//Check criteria for activation.
	}
}

void UNPCBehavior::StopBehavior()
{
	switch (CombatStatus)
	{
	case ENPCCombatStatus::Patrolling :
		//TODO: Leave patrolling state.
		break;
	case ENPCCombatStatus::Combat :
		//TODO: Leave combat state.
		break;
	case ENPCCombatStatus::Resetting :
		//TODO: Leave resetting state?
		break;
	default:
		break;
	}
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
	}
}

void UNPCBehavior::EnterPatrolState()
{
	CombatStatus = ENPCCombatStatus::Patrolling;
	if (Patrol.Num() < 1)
	{
		//This NPC doesn't need to patrol.
		return;
	}
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
	StartPatrol();
}

void UNPCBehavior::StartPatrol()
{
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
				bFinishedPatrolling = true;
				break;
			}
		}
		else
		{
			NextPatrolIndex += 1;
		}
		if (NextPatrolIndex == StartingIndex)
		{
			bFinishedPatrolling = true;
			break;
		}
	}
	if (bFinishedPatrolling)
	{
		return;
	}
	EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(Patrol[NextPatrolIndex].Point->GetActorLocation());
	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful :
		AIController->ReceiveMoveCompleted.AddDynamic(this, &UNPCBehavior::OnPatrolComplete);
		break;
	default :
		OnPatrolComplete(//TODO: No idea what to put here?);
		break;
	}
}



