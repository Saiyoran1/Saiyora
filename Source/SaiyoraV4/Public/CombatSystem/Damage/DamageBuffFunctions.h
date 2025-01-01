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

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Health Event")
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (GetOptions = "GetHealthEventModifierFunctionNames"))
	FName ModifierFunctionName;
	
	FHealthEventModCondition Modifier;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	UFUNCTION()
	TArray<FName> GetHealthEventModifierFunctionNames() const;
	UFUNCTION()
	FCombatModifier ExampleModifierFunction(const FHealthEvent& Event) const { return FCombatModifier(); }
};

UCLASS()
class SAIYORAV4_API UHealthEventRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere, Category = "Health Event")
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;
	UPROPERTY(EditAnywhere, Category = "Health Event", meta = (GetOptions = "GetHealthEventRestrictionFunctionNames"))
	FName RestrictionFunctionName;

	FHealthEventRestriction Restriction;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	UFUNCTION()
	TArray<FName> GetHealthEventRestrictionFunctionNames() const;
	UFUNCTION()
	bool ExampleRestrictionFunction(const FHealthEvent& Event) const { return false; }
};

UCLASS()
class SAIYORAV4_API UDeathRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Death Event", meta = (GetOptions = "GetDeathEventRestrictionFunctionNames"))
	FName RestrictionFunctionName;

	FDeathRestriction Restriction;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;

	UFUNCTION()
	TArray<FName> GetDeathEventRestrictionFunctionNames() const;
	UFUNCTION()
	bool ExampleRestrictionEvent(const FHealthEvent& Event) const { return false; }
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