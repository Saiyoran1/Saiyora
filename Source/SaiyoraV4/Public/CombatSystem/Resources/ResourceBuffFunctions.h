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

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Resource Modifier")
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditAnywhere, Category = "Resource Modifier", meta = (GetOptions = "GetResourceDeltaModifierFunctionNames"))
	FName ModifierFunctionName;
	
	FResourceDeltaModifier Modifier;
	UPROPERTY()
	UResource* TargetResource = nullptr;

	UFUNCTION()
	TArray<FName> GetResourceDeltaModifierFunctionNames() const;
	UFUNCTION()
	FCombatModifier ExampleModifierFunction(UResource* Resource, UObject* Source, const float InitialDelta) const { return FCombatModifier(); }
};
