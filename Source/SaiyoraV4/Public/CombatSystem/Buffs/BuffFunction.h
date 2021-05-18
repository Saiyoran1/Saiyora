// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BuffStructs.h"
#include "DamageEnums.h"
#include "GameplayTagContainer.h"
#include "StatStructs.h"
#include "BuffFunction.generated.h"

UCLASS(Abstract)
class SAIYORAV4_API UBuffFunction : public UObject
{
	GENERATED_BODY()

private:

	UPROPERTY()
	UBuff* OwningBuff = nullptr;
	bool bBuffFunctionInitialized = false;

public:

	static UBuffFunction* InstantiateBuffFunction(UBuff* Buff, TSubclassOf<UBuffFunction> const FunctionClass);
	virtual UWorld* GetWorld() const override;
	void InitializeBuffFunction(UBuff* BuffRef);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* GetOwningBuff() const { return OwningBuff; }

	virtual void SetupBuffFunction() { return; }
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) { return; }
	virtual void CleanupBuffFunction() { return; }
};

//Subclass of UBuffFunction that exposes functionality to Blueprints.
UCLASS(Abstract, Blueprintable)
class UCustomBuffFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunction(UBuff* Buff, TSubclassOf<UCustomBuffFunction> const FunctionClass);
	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunctions(UBuff* Buff, TArray<TSubclassOf<UCustomBuffFunction>> const& FunctionClasses);
	
	virtual void SetupBuffFunction() override { CustomSetup(); }
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override { CustomApply(); }
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override { CustomStack(); }
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) override { CustomRefresh(); }
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override { CustomRemove(); }
	virtual void CleanupBuffFunction() override { CustomCleanup(); }

protected:

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Setup"))
	void CustomSetup();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Apply"))
	void CustomApply();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Stack"))
	void CustomStack();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Refresh"))
	void CustomRefresh();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Remove"))
	void CustomRemove();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Cleanup"))
	void CustomCleanup();
};

//Commonly used buff functions.

UCLASS()
class UDamageOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	float BaseDamage = 0.0f;
	EDamageSchool DamageSchool = EDamageSchool::None;
	float DamageInterval = 0.0f;
	bool bIgnoresRestrictions = false;
	bool bIgnoresModifiers = false;
	bool bSnapshots = false;
	bool bScalesWithStacks = true;
	bool bTicksOnExpire = false;
	
	bool bHasInitialTick = false;
	bool bUsesSeparateInitialDamage = false;
	float InitialDamageAmount = 0.0f;
	EDamageSchool InitialDamageSchool = EDamageSchool::None;
	
	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickDamage();

	void SetDamageVars(float const Damage, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshot, bool const bScaleWithStacks, bool const bPartialTickOnExpire,
		bool const bInitialTick, bool const bUseSeparateInitialDamage, float const InitialDamage,
		EDamageSchool const InitialSchool);
	
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageOverTime(UBuff* Buff, float const Damage, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshots, bool const bScalesWithStacks, bool const bPartialTickOnExpire,
		bool const bHasInitialTick, bool const bUseSeparateInitialDamage, float const InitialDamage,
		EDamageSchool const InitialSchool);
};

UCLASS()
class UHealingOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	float BaseHealing = 0.0f;
	EDamageSchool HealingSchool = EDamageSchool::None;
	float HealingInterval = 0.0f;
	bool bIgnoresRestrictions = false;
	bool bIgnoresModifiers = false;
	bool bSnapshots = false;
	bool bScalesWithStacks = true;
	bool bTicksOnExpire = false;

	bool bHasInitialTick = false;
	bool bUsesSeparateInitialHealing = false;
	float InitialHealingAmount = 0.0f;
	EDamageSchool InitialHealingSchool = EDamageSchool::None;

	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickHealing();

	void SetHealingVars(float const Healing, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshot, bool const bScaleWithStacks, bool const bPartialTickOnExpire,
		bool const bInitialTick, bool const bUseSeparateInitialHealing, float const InitialHealing,
		EDamageSchool const InitialSchool);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void HealingOverTime(UBuff* Buff, float const Healing, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshots, bool const bScalesWithStacks, bool const bPartialTickOnExpire,
		bool const bHasInitialTick, bool const bUseSeparateInitialHealing, float const InitialHealing,
		EDamageSchool const InitialSchool);
};

UCLASS()
class UStatModifierFunction : public UBuffFunction
{
	GENERATED_BODY()
	
	TArray<FStatModifier> StatMods;
	UPROPERTY()
	UStatHandler* TargetHandler = nullptr;

	void SetModifierVars(TArray<FStatModifier> const& Modifiers);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat Modifiers", meta = (DefaultToSelf = "Buff", HidePin = "Buff", GameplayTagFilter = "Stat"))
	static void StatModifiers(UBuff* Buff, TArray<FStatModifier> const& Modifiers);
};