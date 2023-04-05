#include "NPC/NPCBehavior.h"
#include "AIController.h"
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
		//Run behavior tree for either trash or boss?
		APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
		if (IsValid(OwnerAsPawn))
		{
			AAIController* AIController = Cast<AAIController>(OwnerAsPawn->GetController());
			if (IsValid(AIController))
			{
				if (IsValid(BehaviorTree))
				{
					AIController->RunBehaviorTree(BehaviorTree);
					AIController->GetBlackboardComponent()->SetValueAsBool("bShouldPatrol", ShouldPatrol());
					if (IsValid(CombatTree))
					{
						UBehaviorTreeComponent* BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(AIController->GetBrainComponent());
						if (IsValid(BehaviorTreeComponent))
						{
							BehaviorTreeComponent->SetDynamicSubtree(FGameplayTag::RequestGameplayTag(FName(TEXT("Behavior.Combat")), false), CombatTree);
						}
					}
				}
			}
		}
		//Set patrol path blackboard keys?
		//Set combat subtree?
	}
}


