#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "BuffFunction.h"
#include "AbilityBuffFunctions.generated.h"

//Buff functions that apply modifiers or restrictions to the ability system.

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