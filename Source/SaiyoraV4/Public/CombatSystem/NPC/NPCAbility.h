#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "NPCStructs.h"
#include "NPCAbility.generated.h"

UCLASS()
class SAIYORAV4_API UNPCAbility : public UCombatAbility
{
	GENERATED_BODY()

public:

	void GetAbilityLocationRules(FNPCAbilityLocationRules& OutLocationRules);

	//Get the minimum range of this ability. Note that this may not always be range from a target, it can also be range from a specified location or other actor.
	//Returns -1.0f if no minimum range is required.
	UFUNCTION(BlueprintPure)
	float GetMinimumRange() const { return LocationRules.LocationRuleType != ELocationRuleReferenceType::None && LocationRules.bEnforceMinimumRange ? LocationRules.MinimumRange : -1.0f; }
	//Get the maximum range of this ability. Note that this may not always be range from a target, it can also be range from a specified location or other actor.
	//Returns -1.0f if no maximum range is required.
	UFUNCTION(BlueprintPure)
	float GetMaximumRange() const { return LocationRules.LocationRuleType != ELocationRuleReferenceType::None && LocationRules.bEnforceMaximumRange ? LocationRules.MaximumRange : -1.0f; }

protected:

	//Blueprint function that allows an ability to return where it wants to judge its location rules from.
	//For example, if an ability has a minimum range, that can be a minimum range from either an actor or a location.
	//This is determined by the AbilityLocationRules field, and the actual target actor or location is generated here per use of the ability.
	UFUNCTION(BlueprintNativeEvent)
	void GetAbilityLocationRuleOrigin(AActor*& OutTargetActor, FVector& OutTargetLocation) const;
	void GetAbilityLocationRuleOrigin_Implementation(AActor*& OutTargetActor, FVector& OutTargetLocation) const { OutTargetActor = nullptr; OutTargetLocation = FVector::ZeroVector; }

private:

	//Determines the location requirements for an NPC to use this ability.
	UPROPERTY(EditAnywhere, Category = "NPC")
	FNPCAbilityLocationRules LocationRules;
};
