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

UCLASS()
class SAIYORAV4_API UDamageRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDamageRestriction Restrict;
	EDamageModifierType RestrictType;
	UPROPERTY()
	class UDamageHandler* TargetHandler;

	void SetRestrictionVars(EDamageModifierType const RestrictionType, FDamageRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageRestriction(UBuff* Buff, EDamageModifierType const RestrictionType, FDamageRestriction const& Restriction);
};

UCLASS()
class SAIYORAV4_API UDeathRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDeathRestriction Restrict;
	UPROPERTY()
	class UDamageHandler* TargetHandler;

	void SetRestrictionVars(FDeathRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DeathRestriction(UBuff* Buff, FDeathRestriction const& Restriction);
};