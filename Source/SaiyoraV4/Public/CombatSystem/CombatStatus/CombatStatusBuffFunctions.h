#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "CombatStatusStructs.h"
#include "CombatStatusBuffFunctions.generated.h"

UCLASS()
class SAIYORAV4_API UPlaneSwapRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()
	
	UPROPERTY()
	UCombatStatusComponent* TargetComponent = nullptr;

	void SetRestrictionVars();

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void PlaneSwapRestriction(UBuff* Buff);
};
