#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

UENUM(BlueprintType)
enum class EDamageModifierType : uint8
{
	None,
	IncomingDamage,
	OutgoingDamage,
	IncomingHealing,
	OutgoingHealing,
};

UCLASS()
class SAIYORAV4_API UDamageModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDamageModCondition Mod;
	EDamageModifierType ModType;
	UPROPERTY()
	class UDamageHandler* TargetHandler;

	void SetModifierVars(EDamageModifierType const ModifierType, FDamageModCondition const& Modifier);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageModifier(UBuff* Buff, EDamageModifierType const ModifierType, FDamageModCondition const& Modifier);
};
