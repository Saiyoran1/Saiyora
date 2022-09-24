#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

class UDamageHandler;

UCLASS()
class UPeriodicHealthEventFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UDamageHandler* TargetComponent = nullptr;
	UPROPERTY()
	UDamageHandler* GeneratorComponent = nullptr;

	EHealthEventType EventType = EHealthEventType::None;
	float BaseValue = 0.0f;
	EHealthEventSchool EventSchool = EHealthEventSchool::None;
	float EventInterval = 0.0f;
	bool bBypassesAbsorbs = false;
	bool bIgnoresRestrictions = false;
	bool bIgnoresModifiers = false;
	bool bSnapshots = false;
	bool bScalesWithStacks = true;
	bool bTicksOnExpire = false;
	
	bool bHasInitialTick = false;
	bool bUsesSeparateInitialValue = false;
	float InitialValue = 0.0f;
	EHealthEventSchool InitialEventSchool = EHealthEventSchool::None;

	FThreatFromDamage ThreatInfo;

	FHealthEventCallback Callback;
	
	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickHealthEvent();

	void SetEventVars(const EHealthEventType HealthEventType, const float Amount, const EHealthEventSchool School,
		const float Interval, const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshot, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialAmount, const float InitialAmount,
		const EHealthEventSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback);
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (DefaultToSelf = "Buff", HidePin = "Buff", AutoCreateRefTerm = "ThreatParams, TickCallback"))
	static void PeriodicHealthEvent(UBuff* Buff, const EHealthEventType EventType, const float Amount, const EHealthEventSchool School,
		const float Interval, const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshots, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialAmount, const float InitialAmount,
		const EHealthEventSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback);
};

UCLASS()
class SAIYORAV4_API UHealthEventModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	ECombatEventDirection EventDirection = ECombatEventDirection::None;
	FHealthEventModCondition Mod;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	void SetModifierVars(const ECombatEventDirection HealthEventDirection, const FHealthEventModCondition& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void HealthEventModifier(UBuff* Buff, const ECombatEventDirection HealthEventDirection, const FHealthEventModCondition& Modifier);
};

UCLASS()
class SAIYORAV4_API UHealthEventRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FHealthEventRestriction Restrict;
	ECombatEventDirection EventDirection = ECombatEventDirection::None;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	void SetRestrictionVars(const ECombatEventDirection HealthEventDirection, const FHealthEventRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void HealthEventRestriction(UBuff* Buff, const ECombatEventDirection HealthEventDirection, const FHealthEventRestriction& Restriction);
};

UCLASS()
class SAIYORAV4_API UDeathRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDeathRestriction Restrict;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	void SetRestrictionVars(const FDeathRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DeathRestriction(UBuff* Buff, const FDeathRestriction& Restriction);
};