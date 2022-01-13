#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "PlaneStructs.h"
#include "PlaneBuffFunctions.generated.h"

UCLASS()
class SAIYORAV4_API UPlaneSwapRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FPlaneSwapRestriction Restrict;
	UPROPERTY()
	UPlaneComponent* TargetComponent;

	void SetRestrictionVars(FPlaneSwapRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void PlaneSwapRestriction(UBuff* Buff, FPlaneSwapRestriction const& Restriction);
};
