#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "CrowdControlEnums.h"
#include "CrowdControlBuffFunctions.generated.h"

class UCrowdControlHandler;

UCLASS()
class SAIYORAV4_API UCrowdControlFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

	ECrowdControl GetCrowdControlType() const { return CrowdControl; }

private:

	//The type of crowd control to apply.
	UPROPERTY(EditAnywhere)
	ECrowdControl CrowdControl;

	UPROPERTY()
	UCrowdControlHandler* TargetHandler;
};

UCLASS()
class SAIYORAV4_API UCrowdControlImmunityFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	//The types of crowd control to immune.
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/SaiyoraV4.ECrowdControl"))
    int32 ImmuneCcs = 0;
	
    UPROPERTY()
    UCrowdControlHandler* TargetHandler = nullptr;
};
