#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "CombatStatusBuffFunctions.generated.h"

class UCombatStatusComponent;

//A buff function that restricts plane swapping while the buff is applied.
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
