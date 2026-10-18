#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "ThreatStructs.h"
#include "ThreatBuffFunctions.generated.h"

class UThreatHandler;

UCLASS()
class SAIYORAV4_API UThreatModifierFunction : public UBuffFunction
{
	GENERATED_BODY()
	
public:

	UThreatModifierFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere)
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;

	UPROPERTY(Transient)
	FThreatModCondition Modifier;
	UPROPERTY()
	UThreatHandler* TargetHandler;
};

UCLASS()
class SAIYORAV4_API UThreatRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UThreatRestrictionFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:
	
	UPROPERTY(EditAnywhere)
	ECombatEventDirection EventDirection = ECombatEventDirection::Incoming;

	UPROPERTY(Transient)
	FThreatRestriction Restriction;
	UPROPERTY()
	UThreatHandler* TargetHandler;
};

UCLASS()
class SAIYORAV4_API UFadeFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

private:

	UPROPERTY()
	UThreatHandler* TargetHandler;
};

UCLASS()
class SAIYORAV4_API UFixateFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

private:

	AActor* TryGetFixateTarget(const FBuffApplyEvent& ApplyEvent) const;
	UPROPERTY()
	UThreatHandler* TargetHandler;
	UPROPERTY()
	AActor* FixateTarget;
};

UCLASS()
class SAIYORAV4_API UBlindFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

private:

	AActor* TryGetBlindTarget(const FBuffApplyEvent& ApplyEvent) const;
	UPROPERTY()
	UThreatHandler* TargetHandler;
	UPROPERTY()
	AActor* BlindTarget;
};

UCLASS()
class SAIYORAV4_API UMisdirectFunction : public UBuffFunction
{
	GENERATED_BODY()

public:
	
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const override;

private:

	AActor* TryGetMisdirectTarget(const FBuffApplyEvent& ApplyEvent) const;
	UPROPERTY()
	UThreatHandler* TargetHandler;
	UPROPERTY()
	AActor* MisdirectTarget;
};