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
	TMap<FGameplayTag, FCombatModifierHandle> StatModHandles;
	UPROPERTY()
	class UStatHandler* TargetHandler = nullptr;

	void SetModifierVars(const TMap<FGameplayTag, FCombatModifier>& Modifiers);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Stat Modifiers", meta = (DefaultToSelf = "Buff", HidePin = "Buff", GameplayTagFilter = "Stat"))
	static void StatModifiers(UBuff* Buff, const TMap<FGameplayTag, FCombatModifier>& Modifiers);
};