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

	FResourceDeltaModifier Mod;
	UPROPERTY()
	UResource* TargetResource = nullptr;

	void SetModifierVars(const TSubclassOf<UResource> ResourceClass, const FResourceDeltaModifier& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	//Factory function for creating a UResourceDeltaModifierFunction that will manage modifiers to non-ability cost resource changes.
	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ResourceDeltaModifier(UBuff* Buff, const TSubclassOf<UResource> ResourceClass, const FResourceDeltaModifier& Modifier);
};
