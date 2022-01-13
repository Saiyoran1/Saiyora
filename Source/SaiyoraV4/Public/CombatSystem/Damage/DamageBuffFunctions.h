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
class UDamageOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	class UDamageHandler* TargetComponent;
	UPROPERTY()
	class UDamageHandler* GeneratorComponent;

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

	void SetDamageVars(float const Damage, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshot, bool const bScaleWithStacks, bool const bPartialTickOnExpire,
		bool const bInitialTick, bool const bUseSeparateInitialDamage, float const InitialDamage,
		EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams);
	
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void DamageOverTime(UBuff* Buff, float const Damage, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshots, bool const bScalesWithStacks, bool const bPartialTickOnExpire,
		bool const bHasInitialTick, bool const bUseSeparateInitialDamage, float const InitialDamage,
		EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams);
};

UCLASS()
class UHealingOverTimeFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	class UDamageHandler* TargetComponent;
	UPROPERTY()
	class UDamageHandler* GeneratorComponent;
	
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

	void SetHealingVars(float const Healing, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshot, bool const bScaleWithStacks, bool const bPartialTickOnExpire,
		bool const bInitialTick, bool const bUseSeparateInitialHealing, float const InitialHealing,
		EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing Over Time", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void HealingOverTime(UBuff* Buff, float const Healing, EDamageSchool const School,
		float const Interval, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
		bool const bSnapshots, bool const bScalesWithStacks, bool const bPartialTickOnExpire,
		bool const bHasInitialTick, bool const bUseSeparateInitialHealing, float const InitialHealing,
		EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams);
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