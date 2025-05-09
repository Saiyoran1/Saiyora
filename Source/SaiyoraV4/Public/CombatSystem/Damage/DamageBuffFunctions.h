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
	void ExampleCallbackEvent(const FHealthEvent& Event) {}
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
	bool ExampleRestrictionFunction(const FHealthEvent& Event) const { return false; }
};

UCLASS()
class SAIYORAV4_API UPendingResurrectionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	friend struct FPendingResurrection;

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	FVector ResLocation;
	UPROPERTY(EditAnywhere, Category = "Res Event", meta = (InlineEditConditionToggle))
	bool bOverrideHealth = false;
	UPROPERTY(EditAnywhere, Category = "Res Event", meta = (EditCondition = "bOverrideHealth"))
	float HealthPercentageOverride = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Res Event", meta = (GetOptions = "GetResCallbackFunctionNames"))
	FName CallbackFunctionName;
	
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
	int ResID = -1;

	UFUNCTION()
	TArray<FName> GetResCallbackFunctionNames() const;
	UFUNCTION()
	void ExampleCallbackFunction(const FVector& Location) const {}
};