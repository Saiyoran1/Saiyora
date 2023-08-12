#pragma once
#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "Object.h"
#include "CombatGroup.generated.h"

class UThreatHandler;

UCLASS()
class SAIYORAV4_API UCombatGroup : public UObject
{
	GENERATED_BODY()

public:
	
	void AddCombatant(UThreatHandler* Combatant);
	void RemoveCombatant(UThreatHandler* Combatant);

private:
	
	UPROPERTY()
	TArray<UThreatHandler*> Friendlies;
	UPROPERTY()
	TArray<UThreatHandler*> Enemies;

	UFUNCTION()
	void OnCombatantHealthEvent(const FHealthEvent& Event);
	UFUNCTION()
	void OnCombatantLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
};
