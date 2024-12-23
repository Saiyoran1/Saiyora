#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "CombatStatusBuffFunctions.generated.h"

class UCombatStatusComponent;

//A buff function that restricts plane swapping while the buff is applied.
UCLASS()
class SAIYORAV4_API UPlaneSwapRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY()
	UCombatStatusComponent* TargetComponent = nullptr;
};
