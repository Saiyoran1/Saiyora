#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "GameplayTagContainer.h"
#include "CrowdControlBuffFunctions.generated.h"

class UCrowdControlHandler;

UCLASS()
class SAIYORAV4_API UCrowdControlImmunityFunction : public UBuffFunction
{
	GENERATED_BODY()

    FGameplayTag ImmuneCc;
	UPROPERTY()
	UBuffHandler* TargetBuffHandler = nullptr;
    UPROPERTY()
    UCrowdControlHandler* TargetCcHandler = nullptr;
	FBuffRestriction CcImmunity;
	UFUNCTION()
	bool RestrictCcBuff(const FBuffApplyEvent& ApplyEvent);
    
    void SetRestrictionVars(const FGameplayTag ImmunedCc);
    
    virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
    virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
    
    UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (GameplayTagFilter = "CrowdControl", DefaultToSelf = "Buff", HidePin = "Buff"))
    static void CrowdControlImmunity(UBuff* Buff, const FGameplayTag ImmunedCrowdControl);
};
