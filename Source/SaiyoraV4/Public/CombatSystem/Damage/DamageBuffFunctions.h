#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

class UDamageHandler;

UENUM(BlueprintType)
enum class EDamageModifierType : uint8
{
	None = 0,
	IncomingDamage = 1,
	OutgoingDamage = 2,
	IncomingHealing = 3,
	OutgoingHealing = 4,
};

UCLASS()
class UDamageOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UDamageHandler* TargetComponent = nullptr;
	UPROPERTY()
	UDamageHandler* GeneratorComponent = nullptr;

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

	FThreatFromDamage ThreatInfo;
	
	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickDamage();

	void SetDamageVars(const float Damage, const EDamageSchool School,
		const float Interval, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshot, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialDamage, const float InitialDamage,
		const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams);
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageOverTime(UBuff* Buff, const float Damage, const EDamageSchool School,
		const float Interval, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshots, const bool bScalesWithStacks, const bool bPartialTickOnExpire,
		const bool bHasInitialTick, const bool bUseSeparateInitialDamage, const float InitialDamage,
		const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams);
};

UCLASS()
class UHealingOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UDamageHandler* TargetComponent;
	UPROPERTY()
	UDamageHandler* GeneratorComponent;
	
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

	FThreatFromDamage ThreatInfo;

	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickHealing();

	void SetHealingVars(const float Healing, const EDamageSchool School,
		const float Interval, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshot, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialHealing, const float InitialHealing,
		const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void HealingOverTime(UBuff* Buff, const float Healing, const EDamageSchool School,
		const float Interval, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshots, const bool bScalesWithStacks, const bool bPartialTickOnExpire,
		const bool bHasInitialTick, const bool bUseSeparateInitialHealing, const float InitialHealing,
		const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams);
};

UCLASS()
class SAIYORAV4_API UDamageModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDamageModCondition Mod;
	EDamageModifierType ModType = EDamageModifierType::None;
	UPROPERTY()
	UDamageHandler* TargetHandler;

	void SetModifierVars(const EDamageModifierType ModifierType, const FDamageModCondition& Modifier);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageModifier(UBuff* Buff, const EDamageModifierType ModifierType, const FDamageModCondition& Modifier);
};

UCLASS()
class SAIYORAV4_API UDamageRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDamageRestriction Restrict;
	EDamageModifierType RestrictType = EDamageModifierType::None;
	UPROPERTY()
	UDamageHandler* TargetHandler;

	void SetRestrictionVars(const EDamageModifierType RestrictionType, const FDamageRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageRestriction(UBuff* Buff, const EDamageModifierType RestrictionType, const FDamageRestriction& Restriction);
};

UCLASS()
class SAIYORAV4_API UDeathRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FDeathRestriction Restrict;
	UPROPERTY()
	UDamageHandler* TargetHandler;

	void SetRestrictionVars(const FDeathRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DeathRestriction(UBuff* Buff, const FDeathRestriction& Restriction);
};