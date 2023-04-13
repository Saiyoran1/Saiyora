#include "NPC/NPCBehavior.h"
#include "AIController.h"
#include "CombatStructs.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"
#include "BehaviorTree/BlackboardComponent.h"

UNPCBehavior::UNPCBehavior()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPCBehavior::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_Authority)
	{
		const APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
		if (IsValid(OwnerAsPawn))
		{
			Controller = Cast<AAIController>(OwnerAsPawn->GetController());
			if (IsValid(Controller) && IsValid(BehaviorTree))
			{
				UDamageHandler* DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
				if (IsValid(DamageHandler))
				{
					//If there's a damage handler, we subscribe to life status updates.
					DamageHandler->OnLifeStatusChanged.AddDynamic(this, &UNPCBehavior::UpdateBehaviorOnLifeStatusChanged);
				}
				const ELifeStatus InitialLifeStatus = IsValid(DamageHandler) ? DamageHandler->GetLifeStatus() : ELifeStatus::Alive;
				UpdateBehaviorOnLifeStatusChanged(GetOwner(), InitialLifeStatus, InitialLifeStatus);
			}
		}
	}
}

void UNPCBehavior::UpdateBehaviorOnLifeStatusChanged(AActor* Target, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (!IsValid(Controller))
	{
		return;
	}
	if (NewStatus == ELifeStatus::Alive)
	{
		//If the NPC is coming alive (either spawning or respawning), run the behavior tree, set the combat subtree, and set up subscriptions for blackboard variables.
		Controller->RunBehaviorTree(BehaviorTree);
		if (IsValid(CombatTree))
		{
			UBehaviorTreeComponent* BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());
			if (IsValid(BehaviorTreeComponent))
			{
				BehaviorTreeComponent->SetDynamicSubtree(FSaiyoraCombatTags::Get().Behavior_Combat, CombatTree);
			}
		}
		Blackboard = Controller->GetBlackboardComponent();
		if (IsValid(Blackboard))
		{
			Blackboard->SetValueAsBool("bShouldPatrol", ShouldPatrol());
			Blackboard->SetValueAsFloat("PatrolMoveSpeedMod", PatrolMoveSpeedModifier);
			Blackboard->SetValueAsVector("ResetGoal", GetOwner()->GetActorLocation());
			UThreatHandler* ThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
			if (IsValid(ThreatHandler))
			{
				//If there is a threat handler, we subscribe to combat status updates and target updates.
				//TODO: Are these subscriptions unique? For respawning, will we get multiple calls?
				ThreatHandler->OnCombatChanged.AddDynamic(this, &UNPCBehavior::UpdateBehaviorOnCombatChanged);
				ThreatHandler->OnTargetChanged.AddDynamic(this, &UNPCBehavior::UpdateBehaviorOnTargetChanged);
				Blackboard->SetValueAsBool("bInCombat", ThreatHandler->IsInCombat());
				Blackboard->SetValueAsObject("Target", ThreatHandler->GetCurrentTarget());
			}
			else
			{
				//If no threat handler, we don't run the combat tree at all.
				Blackboard->SetValueAsBool("bInCombat", false);
				Blackboard->SetValueAsObject("Target", nullptr);
			}
		}
	}
	else if (IsValid(Controller->GetBrainComponent()))
	{
		//If the NPC is dying or becoming inactive, simply stop the current behavior tree if one exists.
		Controller->GetBrainComponent()->StopLogic(TEXT("Death"));
		Blackboard = nullptr;
	}
}

void UNPCBehavior::UpdateBehaviorOnCombatChanged(const bool bInCombat)
{
	if (!IsValid(Blackboard))
	{
		return;
	}
	Blackboard->SetValueAsBool("bInCombat", bInCombat);
}

void UNPCBehavior::UpdateBehaviorOnTargetChanged(AActor* PreviousTarget, AActor* NewTarget)
{
	if (!IsValid(Blackboard))
	{
		return;
	}
	Blackboard->SetValueAsObject("Target", NewTarget);
}


