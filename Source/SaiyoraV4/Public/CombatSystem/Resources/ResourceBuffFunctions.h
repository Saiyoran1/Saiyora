#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "ResourceStructs.h"
#include "ResourceBuffFunctions.generated.h"

UCLASS()
class SAIYORAV4_API UResourceDeltaModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FResourceDeltaModifier Mod;
	UPROPERTY()
	class UResource* TargetResource;

	void SetModifierVars(TSubclassOf<UResource> const ResourceClass, FResourceDeltaModifier const& Modifier);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ResourceDeltaModifier(UBuff* Buff, TSubclassOf<UResource> const ResourceClass, FResourceDeltaModifier const& Modifier);
};
