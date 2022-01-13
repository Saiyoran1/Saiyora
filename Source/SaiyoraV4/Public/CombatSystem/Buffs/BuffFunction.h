#pragma once
#include "BuffStructs.h"
#include "BuffFunction.generated.h"

class UBuffHandler;

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

	virtual void SetupBuffFunction() {}
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) {}
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) {}
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) {}
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) {}
	virtual void CleanupBuffFunction() {}
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
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override { CustomApply(ApplyEvent); }
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override { CustomStack(ApplyEvent); }
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) override { CustomRefresh(ApplyEvent); }
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override { CustomRemove(RemoveEvent); }
	virtual void CleanupBuffFunction() override { CustomCleanup(); }

protected:

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Setup"))
	void CustomSetup();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Apply"))
	void CustomApply(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Stack"))
	void CustomStack(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Refresh"))
	void CustomRefresh(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Remove"))
	void CustomRemove(FBuffRemoveEvent const& RemoveEvent);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Cleanup"))
	void CustomCleanup();
};

UCLASS()
class UBuffRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	EBuffRestrictionType RestrictType;
	FBuffRestriction Restrict;
	UPROPERTY()
	UBuffHandler* TargetComponent;

	void SetRestrictionVars(EBuffRestrictionType const RestrictionType, FBuffRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void BuffRestriction(UBuff* Buff, EBuffRestrictionType const RestrictionType, FBuffRestriction const& Restriction);
};