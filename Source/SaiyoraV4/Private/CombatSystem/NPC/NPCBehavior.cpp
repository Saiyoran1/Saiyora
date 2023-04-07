#include "NPC/NPCBehavior.h"
#include "AIController.h"
#include "CombatStructs.h"
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
			if (IsValid(Controller))
			{
				if (IsValid(BehaviorTree))
				{
					Controller->RunBehaviorTree(BehaviorTree);
					UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
					if (IsValid(Blackboard))
					{
						Blackboard->SetValueAsBool("bShouldPatrol", ShouldPatrol());
					}
					if (IsValid(CombatTree))
					{
						UBehaviorTreeComponent* BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());
						if (IsValid(BehaviorTreeComponent))
						{
							BehaviorTreeComponent->SetDynamicSubtree(FSaiyoraCombatTags::Get().Behavior_Combat, CombatTree);
						}
					}
				}
			}
		}
	}
}


