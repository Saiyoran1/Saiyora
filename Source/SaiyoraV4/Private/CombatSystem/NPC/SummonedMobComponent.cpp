#include "SummonedMobComponent.h"

#include "DamageHandler.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"

USummonedMobComponent::USummonedMobComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USummonedMobComponent::Initialize(AActor* Spawner)
{
	if (!IsValid(Spawner))
	{
		Despawn();
		return;
	}
	SpawningActor = Spawner;
	if (SpawningActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		SpawningActorDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(SpawningActor);
		SpawningActorNPCComponent = Cast<UNPCAbilityComponent>(ISaiyoraCombatInterface::Execute_GetAbilityComponent(SpawningActor));
		if (IsValid(SpawningActorDamageHandler))
		{
			SpawningActorDamageHandler->OnLifeStatusChanged.AddDynamic(this, &USummonedMobComponent::OnSpawnerLifeStatusChanged);
		}
		if (IsValid(SpawningActorNPCComponent))
		{
			SpawningActorNPCComponent->OnCombatBehaviorChanged.AddDynamic(this, &USummonedMobComponent::OnCombatBehaviorChanged);
		}
	}
	
}

void USummonedMobComponent::Despawn()
{
	//TODO: Unsure what, if any, cleanup needs to happen.
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetOwner()->Destroy();
	}
}

void USummonedMobComponent::OnSpawnerLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus,
	const ELifeStatus NewStatus)
{
	if (NewStatus == ELifeStatus::Dead)
	{
		Despawn();
	}
}

void USummonedMobComponent::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousStatus,
	const ENPCCombatBehavior NewStatus)
{
	if (NewStatus == ENPCCombatBehavior::Resetting || NewStatus == ENPCCombatBehavior::Patrolling)
	{
		Despawn();
	}
}


