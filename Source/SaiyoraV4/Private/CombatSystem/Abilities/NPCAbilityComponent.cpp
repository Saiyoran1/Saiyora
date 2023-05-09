#include "NPCAbilityComponent.h"
#include "DamageHandler.h"
#include "NPCBehavior.h"
#include "SaiyoraCombatInterface.h"
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
	Super::InitializeComponent();
}

void UNPCAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
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
			//LeavePatrolState();
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
			//EnterPatrolState();
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
