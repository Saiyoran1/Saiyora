#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "BuffFunctionality.h"
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

UCLASS()
class UComplexAbilityModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UComplexAbilityModifierFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	EComplexAbilityModType ModifierType = EComplexAbilityModType::CastLength;

	UPROPERTY(Transient)
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

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditAnywhere)
	FCombatModifier Modifier;
	UPROPERTY(EditAnywhere)
	ESimpleAbilityModType ModType = ESimpleAbilityModType::None;
	
	FCombatModifierHandle Handle = FCombatModifierHandle::Invalid;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;
};

UCLASS()
class UAbilityCostModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditAnywhere)
	FCombatModifier Modifier;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecificAbility = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bSpecificAbility"))
	TSubclassOf<UCombatAbility> AbilityClass;
	
	UPROPERTY()
	UAbilityComponent* TargetHandler = nullptr;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;
	FCombatModifierHandle Handle = FCombatModifierHandle::Invalid;
};

UCLASS()
class UAbilityClassRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere)
	TSet<TSubclassOf<UCombatAbility>> RestrictClasses;
	
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;
};

UCLASS()
class UAbilityTagRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer RestrictTags;
	
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;
};

UCLASS()
class UInterruptRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UInterruptRestrictionFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(Transient)
	FInterruptRestriction Restriction;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;
};
