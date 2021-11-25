#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "ThreatStructs.h"
#include "CombatGroup.generated.h"

class UThreatHandler;

UCLASS(BlueprintType)
class SAIYORAV4_API UCombatGroup : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Combat")
	void GetActorsInCombat(TArray<AActor*>& OutActors) const { OutActors = Combatants; }
	void JoinCombat(AActor* NewCombatant);
	void LeaveCombat(AActor* LeavingCombatant);
	void MergeGroups(UCombatGroup* OtherGroup);
	void NotifyOfMergeStart() { bInMerge = true; }
	void NotifyOfMergeComplete() { bInMerge = false; Combatants.Empty(); }

private:
	
	UPROPERTY()
	TArray<AActor*> Combatants;
	bool bInMerge = false;
};