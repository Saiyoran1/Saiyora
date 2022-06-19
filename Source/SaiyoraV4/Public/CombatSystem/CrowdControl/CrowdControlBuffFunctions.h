#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "GameplayTagContainer.h"
#include "CrowdControlBuffFunctions.generated.h"

UCLASS()
class SAIYORAV4_API UCrowdControlImmunityFunction : public UBuffFunction
{
	GENERATED_BODY()

    FGameplayTag ImmuneCc;
	UPROPERTY()
	class UBuffHandler* TargetBuffHandler;
    UPROPERTY()
    class UCrowdControlHandler* TargetCcHandler;
	FBuffRestriction CcImmunity;
	UFUNCTION()
	bool RestrictCcBuff(FBuffApplyEvent const& ApplyEvent);
    
    void SetRestrictionVars(FGameplayTag const ImmunedCc);
    
    virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
    virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
    
    UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (GameplayTagFilter = "CrowdControl", DefaultToSelf = "Buff", HidePin = "Buff"))
    static void CrowdControlImmunity(UBuff* Buff, FGameplayTag const ImmunedCrowdControl);
};
