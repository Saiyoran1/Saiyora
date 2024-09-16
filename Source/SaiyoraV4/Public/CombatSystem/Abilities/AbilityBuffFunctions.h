#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "BuffFunction.h"
#include "AbilityBuffFunctions.generated.h"

class UAbilityComponent;

UENUM(BlueprintType)
enum class EComplexAbilityModType : uint8
{
	None,
	CastLength,
	GlobalCooldownLength,
	CooldownLength,
};

USTRUCT()
struct FComplexAbilityMod : public FBuffFunction
{
	GENERATED_BODY()

public:

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditDefaultsOnly)
	EComplexAbilityModType ModifierType = EComplexAbilityModType::CastLength;
	UPROPERTY(EditDefaultsOnly, meta = (GetOptions = "GetComplexAbilityModFunctionNames"))
	FName ModiferFunction;
	
	FAbilityModCondition Modifier;
	UPROPERTY()
	UAbilityComponent* TargetHandler = nullptr;
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
	ESimpleAbilityModType ModType = ESimpleAbilityModType::None;
	FCombatModifierHandle ModHandle = FCombatModifierHandle::Invalid;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;

	void SetModifierVars(const TSubclassOf<UCombatAbility> AbilityClass, const ESimpleAbilityModType ModifierType, const FCombatModifier& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void SimpleAbilityModifier(UBuff* Buff, const TSubclassOf<UCombatAbility> AbilityClass, const ESimpleAbilityModType ModifierType, const FCombatModifier& Modifier);
};

UCLASS()
class UAbilityCostModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FCombatModifier Mod;
	TSubclassOf<UResource> Resource;
	TSubclassOf<UCombatAbility> Ability;
	UPROPERTY()
	UAbilityComponent* TargetHandler = nullptr;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;
	FCombatModifierHandle Handle;

	void SetModifierVars(const TSubclassOf<UResource> ResourceClass, const TSubclassOf<UCombatAbility> AbilityClass, const FCombatModifier& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityCostModifier(UBuff* Buff, const TSubclassOf<UResource> ResourceClass, const TSubclassOf<UCombatAbility> AbilityClass, const FCombatModifier& Modifier);
};

UCLASS()
class UAbilityClassRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	TSet<TSubclassOf<UCombatAbility>> RestrictClasses;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;

	void SetRestrictionVars(const TSet<TSubclassOf<UCombatAbility>>& AbilityClasses);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityClassRestrictions(UBuff* Buff, const TSet<TSubclassOf<UCombatAbility>>& AbilityClasses);
};

UCLASS()
class UAbilityTagRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FGameplayTagContainer RestrictTags;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;

	void SetRestrictionVars(const FGameplayTagContainer& RestrictedTags);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void AbilityTagRestrictions(UBuff* Buff, const FGameplayTagContainer& RestrictedTags);
};

UCLASS()
class UInterruptRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FInterruptRestriction Restrict;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;

	void SetRestrictionVars(const FInterruptRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void InterruptRestriction(UBuff* Buff, const FInterruptRestriction& Restriction);
};