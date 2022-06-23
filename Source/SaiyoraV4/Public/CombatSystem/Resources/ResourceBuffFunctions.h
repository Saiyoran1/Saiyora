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
	UResource* TargetResource = nullptr;

	void SetModifierVars(const TSubclassOf<UResource> ResourceClass, const FResourceDeltaModifier& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ResourceDeltaModifier(UBuff* Buff, const TSubclassOf<UResource> ResourceClass, const FResourceDeltaModifier& Modifier);
};
