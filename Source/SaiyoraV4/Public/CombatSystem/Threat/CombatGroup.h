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
	
	bool AddCombatant(UThreatHandler* Combatant);
	void RemoveCombatant(UThreatHandler* Combatant);
	void UpdateCombatantFadeStatus(const UThreatHandler* Combatant, const bool bFaded);
	
	void MergeWith(UCombatGroup* OtherGroup);
	void NotifyOfMerge();

private:
	
	UPROPERTY()
	TArray<UThreatHandler*> Friendlies;
	UPROPERTY()
	TArray<UThreatHandler*> Enemies;

	UFUNCTION()
	void OnCombatantIncomingHealthEvent(const FHealthEvent& Event);
	UFUNCTION()
	void OnCombatantOutgoingHealthEvent(const FHealthEvent& Event);
	UFUNCTION()
	void OnCombatantLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
};
