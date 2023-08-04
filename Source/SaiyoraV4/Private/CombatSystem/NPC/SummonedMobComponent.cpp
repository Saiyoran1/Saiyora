#include "SummonedMobComponent.h"

#include "DamageHandler.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

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
		UThreatHandler* OwnerThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
		if (IsValid(OwnerThreat) && OwnerThreat->HasThreatTable())
		{
			UThreatHandler* SpawnerThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(SpawningActor);
			if (IsValid(SpawnerThreat) && SpawnerThreat->HasThreatTable())
			{
				TArray<AActor*> Targets;
				SpawnerThreat->GetActorsInThreatTable(Targets);
				for (AActor* Target : Targets)
				{
					OwnerThreat->AddThreat(EThreatType::Absolute, 1.0f, Target, nullptr, true, true, FThreatModCondition());
				}
			}
		}
	}
}

void USummonedMobComponent::Despawn()
{
	//TODO: Unsure what, if any, cleanup needs to happen.
	//TODO: Threat cleanup. Currently the mob won't leave enemy threat tables. This should probably go along with a combat group/threat rework.
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


