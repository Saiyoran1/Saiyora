#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

class ASaiyoraPlayerCharacter;
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
	EElementalSchool EventSchool = EElementalSchool::None;
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
	EElementalSchool InitialEventSchool = EElementalSchool::None;

	FThreatFromDamage ThreatInfo;

	FHealthEventCallback Callback;
	
	FTimerHandle TickHandle;

	void InitialTick();
	UFUNCTION()
	void TickHealthEvent();

	void SetEventVars(const EHealthEventType HealthEventType, const float Amount, const EElementalSchool School,
		const float Interval, const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshot, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialAmount, const float InitialAmount,
		const EElementalSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback);
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (DefaultToSelf = "Buff", HidePin = "Buff", AutoCreateRefTerm = "ThreatParams, TickCallback"))
	static void PeriodicHealthEvent(UBuff* Buff, const EHealthEventType EventType, const float Amount, const EElementalSchool School,
		const float Interval, const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers,
		const bool bSnapshots, const bool bScaleWithStacks, const bool bPartialTickOnExpire,
		const bool bInitialTick, const bool bUseSeparateInitialAmount, const float InitialAmount,
		const EElementalSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback);
};

UCLASS()
class SAIYORAV4_API UHealthEventModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;
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
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;
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

UCLASS()
class SAIYORAV4_API UPendingResurrectionFunction : public UBuffFunction
{
	GENERATED_BODY()
	
	FVector ResLocation;
	bool bOverrideHealth = false;
	float HealthOverride = 1.0f;
	UPROPERTY()
	ASaiyoraPlayerCharacter* TargetPlayer = nullptr;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
	//The owning buff can optionally bind a callback for when the res is accepted or declined.
	//Most resurrection buffs will want to do this to remove themselves upon a decision being made.
	FResDecisionCallback ResDecisionCallback;

	//This callback is for the player class to execute when a decision is made, so that this buff function can trigger the res and subsequently call the owning buff callback.
	FResDecisionCallback InternalResDecisionCallback;
	UFUNCTION()
	void OnResDecision(const bool bAccepted);

	void SetResurrectionVars(const FVector& ResurrectLocation, const bool bOverrideHealthPercent, const float HealthOverridePercent, const FResDecisionCallback& DecisionCallback);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff", AutoCreateRefTerm = "ResDecisionCallback"))
	static void OfferResurrection(UBuff* Buff, const FVector& ResurrectLocation, const bool bOverrideHealthPercent, const float HealthOverridePercent, const FResDecisionCallback& DecisionCallback);
};