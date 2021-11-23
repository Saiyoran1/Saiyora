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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
	void SubscribeToCombatantAdded(FCombatantCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
	void UnsubscribeFromCombatantAdded(FCombatantCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
	void SubscribeToCombatantRemoved(FCombatantCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
	void UnsubscribeFromCombatantRemoved(FCombatantCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Combat")
	void GetActorsInCombat(TArray<AActor*>& OutActors) const { OutActors = Combatants; }
	void JoinCombat(AActor* NewCombatant);
	void LeaveCombat(AActor* Combatant);
	void MergeGroups(UCombatGroup* OtherGroup);

private:
	
	UPROPERTY()
	TArray<AActor*> Combatants;
	FCombatantNotification OnCombatantAdded;
	FCombatantNotification OnCombatantRemoved;
};