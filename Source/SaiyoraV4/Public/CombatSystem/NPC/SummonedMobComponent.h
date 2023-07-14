#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "NPCEnums.h"
#include "Components/ActorComponent.h"
#include "SummonedMobComponent.generated.h"

class UNPCAbilityComponent;
class UDamageHandler;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API USummonedMobComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	USummonedMobComponent();

	void Initialize(AActor* Spawner);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void DespawnMob() { Despawn(); }

private:

	void Despawn();
	UPROPERTY()
	AActor* SpawningActor;
	UPROPERTY()
	UNPCAbilityComponent* SpawningActorNPCComponent;
	UPROPERTY()
	UDamageHandler* SpawningActorDamageHandler;

	UFUNCTION()
	void OnSpawnerLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousStatus, const ENPCCombatBehavior NewStatus);
};
