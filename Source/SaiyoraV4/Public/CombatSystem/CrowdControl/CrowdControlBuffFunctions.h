#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "GameplayTagContainer.h"
#include "CrowdControlBuffFunctions.generated.h"

class UCrowdControlHandler;

UCLASS()
class SAIYORAV4_API UCrowdControlImmunityFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	//The type of CC we are immuning
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (Categories = "CrowdControl"))
    FGameplayTag ImmuneCc;
	
	UPROPERTY()
	UBuffHandler* TargetBuffHandler = nullptr;
    UPROPERTY()
    UCrowdControlHandler* TargetCcHandler = nullptr;
	//Buff restriction function to block buffs applying the CC we are immuning
	FBuffRestriction CcImmunity;
	UFUNCTION()
	bool RestrictCcBuff(const FBuffApplyEvent& ApplyEvent);
};
