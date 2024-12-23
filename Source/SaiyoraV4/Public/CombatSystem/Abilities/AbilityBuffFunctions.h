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

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	EComplexAbilityModType ModifierType = EComplexAbilityModType::CastLength;
	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetComplexAbilityModFunctionNames"))
	FName ModifierFunctionName;
	
	FAbilityModCondition Modifier;
	UPROPERTY()
	UAbilityComponent* TargetHandler = nullptr;

	UFUNCTION()
	TArray<FName> GetComplexAbilityModFunctionNames() const;
	//This function just exists to match other function signatures against to find viable modifier functions.
	UFUNCTION()
	FCombatModifier ExampleModifierFunction(UCombatAbility* Ability) const { return FCombatModifier(); }
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

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	FCombatModifier Modifier;
	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	ESimpleAbilityModType ModType = ESimpleAbilityModType::None;
	
	FCombatModifierHandle ModHandle = FCombatModifierHandle::Invalid;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;
};

UCLASS()
class UAbilityCostModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	FCombatModifier Modifier;
	UPROPERTY(EditAnywhere, Category = "Ability Modifier")
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditAnywhere, Category = "Ability Modifier", meta = (InlineEditConditionToggle))
	bool bSpecificAbility = false;
	UPROPERTY(EditAnywhere, Category = "Ability Modifier", meta = (EditCondition = "bSpecificAbility"))
	TSubclassOf<UCombatAbility> AbilityClass;
	
	UPROPERTY()
	UAbilityComponent* TargetHandler = nullptr;
	UPROPERTY()
	UCombatAbility* TargetAbility = nullptr;
	FCombatModifierHandle Handle;
};

UCLASS()
class UAbilityClassRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere, Category = "Ability Restriction")
	TSet<TSubclassOf<UCombatAbility>> RestrictClasses;
	
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;
};

UCLASS()
class UAbilityTagRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Ability Restriction")
	FGameplayTagContainer RestrictTags;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;
};

UCLASS()
class UInterruptRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Interrupt Restriction", meta = (GetOptions = "GetInterruptRestrictionFunctionNames"))
	FName RestrictionFunctionName;
	
	FInterruptRestriction Restriction;
	UPROPERTY()
	UAbilityComponent* TargetComponent = nullptr;

	UFUNCTION()
	TArray<FName> GetInterruptRestrictionNames() const;
	UFUNCTION()
	bool ExampleRestrictionFunction(const FInterruptEvent& Event) const { return false; }
};
