#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "ResourceStructs.h"
#include "ResourceBuffFunctions.generated.h"

//Buff function for applying modifiers to non-ability cost resource generation and expenditure that last the duration of the buff.
//Modifiers can optionally stack with the buff.
UCLASS()
class SAIYORAV4_API UResourceDeltaModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UResourceDeltaModifierFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UResource> ResourceClass;

	UPROPERTY(Transient)
	FResourceDeltaModifier Modifier;
	UPROPERTY()
	UResource* TargetResource = nullptr;
};
