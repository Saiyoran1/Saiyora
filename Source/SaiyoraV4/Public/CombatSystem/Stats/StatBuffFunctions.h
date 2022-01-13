#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "GameplayTagContainer.h"
#include "StatBuffFunctions.generated.h"

UCLASS()
class UStatModifierFunction : public UBuffFunction
{
	GENERATED_BODY()
	
	TMap<FGameplayTag, FCombatModifier> StatMods;
	UPROPERTY()
	class UStatHandler* TargetHandler = nullptr;

	void SetModifierVars(TMap<FGameplayTag, FCombatModifier> const& Modifiers);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Stat Modifiers", meta = (DefaultToSelf = "Buff", HidePin = "Buff", GameplayTagFilter = "Stat"))
	static void StatModifiers(UBuff* Buff, TMap<FGameplayTag, FCombatModifier> const& Modifiers);
};