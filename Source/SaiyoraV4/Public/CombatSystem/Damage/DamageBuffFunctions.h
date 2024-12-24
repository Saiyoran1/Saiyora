#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

class ASaiyoraPlayerCharacter;
class UDamageHandler;

UCLASS()
class UPeriodicHealthEventFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual void CleanupBuffFunction() override;

private:

	UPROPERTY(EditAnywhere, Category = "Health Event")
	EHealthEventType EventType = EHealthEventType::None;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	float BaseValue = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	EElementalSchool EventSchool = EElementalSchool::None;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bBypassesAbsorbs = false;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bIgnoresRestrictions = false;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bIgnoresModifiers = false;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bSnapshots = false;
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bScalesWithStacks = true;
	
	UPROPERTY(EditAnywhere, Category = "Health Event")
	float EventInterval = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bHasInitialTick = false;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (EditCondition = "bHasInitialTick"))
	bool bUsesSeparateInitialValue = false;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (EditCondition = "bHasInitialTick && bUsesSeparateInitialValue"))
	float InitialValue = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (EditCondition = "bHasInitialTick && bUsesSeparateInitialValue"))
	EElementalSchool InitialEventSchool = EElementalSchool::None;

	UPROPERTY(EditAnywhere, Category = "Health Event")
	bool bTicksOnExpire = false;

	UPROPERTY(EditAnywhere, Category = "Health Event")
	FThreatFromDamage ThreatInfo;

	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (InlineEditConditionToggle))
	bool bHasHealthEventCallback = false;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (GetOptions = "GetHealthEventCallbackFunctionNames", EditCondition = "bHasHealthEventCallback"))
	FName HealthEventCallbackName;

	UFUNCTION()
	TArray<FName> GetHealthEventCallbackFunctionNames() const;
	UFUNCTION()
	void ExampleHealthEventCallback(const FHealthEvent& Event) {}
	FHealthEventCallback Callback;
	
	void InitialTick();
	FTimerHandle TickHandle;
	UFUNCTION()
	void TickHealthEvent();

	UPROPERTY()
	UDamageHandler* TargetComponent = nullptr;
	UPROPERTY()
	UDamageHandler* GeneratorComponent = nullptr;
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