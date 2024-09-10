#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "Object.h"
#include "UIBuffFunctions.generated.h"

class UPlayerHUD;
class UCombatAbility;

UCLASS()
class SAIYORAV4_API UAbilityProcFunction : public UBuffFunction
{
	GENERATED_BODY()

	TSubclassOf<UCombatAbility> TargetAbility;
	UPROPERTY()
	UPlayerHUD* LocalPlayerHUD = nullptr;

	void SetModifierVars(const TSubclassOf<UCombatAbility> AbilityClass);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ProcAbility(UBuff* Buff, const TSubclassOf<UCombatAbility> AbilityClass);
};