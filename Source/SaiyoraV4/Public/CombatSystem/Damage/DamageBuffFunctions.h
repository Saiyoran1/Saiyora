#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "DamageStructs.h"
#include "DamageBuffFunctions.generated.h"

class UDamageHandler;

UCLASS()
class UPeriodicHealthEventFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UPeriodicHealthEventFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

	EHealthEventType GetEventType() const { return EventType; }

private:

	UPROPERTY(EditAnywhere)
	EHealthEventType EventType = EHealthEventType::None;
	UPROPERTY(EditAnywhere)
	float BaseValue = 0.0f;
	UPROPERTY(EditAnywhere)
	EElementalSchool EventSchool = EElementalSchool::None;
	UPROPERTY(EditAnywhere)
	bool bBypassesAbsorbs = false;
	UPROPERTY(EditAnywhere)
	bool bIgnoresRestrictions = false;
	UPROPERTY(EditAnywhere)
	bool bIgnoresModifiers = false;
	UPROPERTY(EditAnywhere)
	bool bSnapshots = false;
	UPROPERTY(EditAnywhere)
	bool bScalesWithStacks = true;
	
	UPROPERTY(EditAnywhere)
	float EventInterval = 0.0f;
	
	UPROPERTY(EditAnywhere)
	bool bHasInitialTick = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHasInitialTick"))
	bool bUsesSeparateInitialValue = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHasInitialTick && bUsesSeparateInitialValue"))
	float InitialValue = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHasInitialTick && bUsesSeparateInitialValue"))
	EElementalSchool InitialEventSchool = EElementalSchool::None;

	UPROPERTY(EditAnywhere)
	bool bTicksOnExpire = false;

	UPROPERTY(EditAnywhere)
	FThreatFromDamage ThreatInfo;
	
	UPROPERTY(Transient)
	FHealthEventCallback Callback;
	
	void InitialTick();
	FTimerHandle TickHandle;
	UFUNCTION()
	void TickHealthEvent();

	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
};

UCLASS()
class SAIYORAV4_API UHealthEventModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UHealthEventModifierFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;

	UPROPERTY(Transient)
	FHealthEventModCondition Modifier;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
};

UCLASS()
class SAIYORAV4_API UHealthEventRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UHealthEventRestrictionFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere)
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;

	UPROPERTY(Transient)
	FHealthEventRestriction Restriction;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
};

UCLASS()
class SAIYORAV4_API UDeathRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UDeathRestrictionFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(Transient)
	FDeathRestriction Restriction;
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
};

UCLASS()
class SAIYORAV4_API UPendingResurrectionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	friend struct FPendingResurrection;
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	FVector ResLocation;
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bOverrideHealth = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bOverrideHealth"))
	float HealthPercentageOverride = 1.0f;
	//TODO: Figure out what's going on here.
	UPROPERTY(EditAnywhere)
	FName CallbackFunctionName;
	
	UPROPERTY()
	UDamageHandler* TargetHandler = nullptr;
	int ResID = -1;
	UFUNCTION()
	void OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
};