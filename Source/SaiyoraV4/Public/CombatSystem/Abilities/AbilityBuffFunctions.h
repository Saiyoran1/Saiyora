#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "BuffFunction.h"
#include "AbilityBuffFunctions.generated.h"

UENUM(BlueprintType)
enum class EComplexAbilityModType : uint8
{
	None,
	CastLength,
	GlobalCooldownLength,
	CooldownLength,
};

UCLASS()
class UComplexAbilityModifierFunction : public UBuffFunction
{
	GENERATED_BODY()
	
	FAbilityModCondition Mod;
	EComplexAbilityModType ModType;
	UPROPERTY()
	class UAbilityComponent* TargetHandler = nullptr;

	void SetModifierVars(EComplexAbilityModType const ModifierType, FAbilityModCondition const& Modifier);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ComplexAbilityModifier(UBuff* Buff, EComplexAbilityModType const ModifierType, FAbilityModCondition const& Modifier);
};

UENUM(BlueprintType)
enum class ESimpleAbilityModType : uint8
{
	None,
	MaxCharges,
	ChargeCost,
	ChargesPerCooldown,
};

UCLASS()
class USimpleAbilityModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FCombatModifier Mod;
	ESimpleAbilityModType ModType;
	UPROPERTY()
	class UCombatAbility* TargetAbility;

	void SetModifierVars(TSubclassOf<UCombatAbility> const AbilityClass, ESimpleAbilityModType const ModifierType, FCombatModifier const& Modifier);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void SimpleAbilityModifier(UBuff* Buff, TSubclassOf<UCombatAbility> const AbilityClass, ESimpleAbilityModType const ModifierType, FCombatModifier const& Modifier);
};

UCLASS()
class UAbilityCostModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FCombatModifier Mod;
	TSubclassOf<UResource> Resource;
	TSubclassOf<UCombatAbility> Ability;
	UPROPERTY()
	class UAbilityComponent* TargetHandler;
	UPROPERTY()
	class UCombatAbility* TargetAbility;

	void SetModifierVars(TSubclassOf<UResource> const ResourceClass, TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityCostModifier(UBuff* Buff, TSubclassOf<UResource> const ResourceClass, TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
};

UCLASS()
class UAbilityClassRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	TSet<TSubclassOf<UCombatAbility>> RestrictClasses;
	UPROPERTY()
	UAbilityComponent* TargetComponent;

	void SetRestrictionVars(TSet<TSubclassOf<UCombatAbility>> const& AbilityClasses);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityClassRestrictions(UBuff* Buff, TSet<TSubclassOf<UCombatAbility>> const& AbilityClasses);
};

UCLASS()
class UAbilityTagRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FGameplayTagContainer RestrictTags;
	UPROPERTY()
	UAbilityComponent* TargetComponent;

	void SetRestrictionVars(FGameplayTagContainer const& RestrictedTags);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityTagRestrictions(UBuff* Buff, FGameplayTagContainer const& RestrictedTags);
};

UCLASS()
class UInterruptRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FInterruptRestriction Restrict;
	UPROPERTY()
	UAbilityComponent* TargetComponent;

	void SetRestrictionVars(FInterruptRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void InterruptRestriction(UBuff* Buff, FInterruptRestriction const& Restriction);
};