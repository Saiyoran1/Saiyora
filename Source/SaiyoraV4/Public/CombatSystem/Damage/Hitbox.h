#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "CombatEnums.h"
#include "NPCEnums.h"
#include "Hitbox.generated.h"

class ASaiyoraGameState;
class UNPCAbilityComponent;
class UCombatStatusComponent;

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UHitbox : public UBoxComponent
{
	GENERATED_BODY()
	
public:

	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
private:

	void UpdateFactionCollision(const EFaction NewFaction);
	UPROPERTY()
	ASaiyoraGameState* GameState = nullptr;
	UPROPERTY()
	UNPCAbilityComponent* NPCComponentRef = nullptr;
	UPROPERTY()
	UCombatStatusComponent* CombatStatusComponentRef = nullptr;

	UFUNCTION()
	void OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior);
};
