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

	static UBuffFunction* InstantiateBuffFunction(UBuff* Buff, const TSubclassOf<UBuffFunction> FunctionClass);
	virtual UWorld* GetWorld() const override;
	void InitializeBuffFunction(UBuff* BuffRef);
	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* GetOwningBuff() const { return OwningBuff; }

	virtual void SetupBuffFunction() {}
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) {}
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) {}
	virtual void OnRefresh(const FBuffApplyEvent& ApplyEvent) {}
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) {}
	virtual void CleanupBuffFunction() {}
};

//Subclass of UBuffFunction that exposes functionality to Blueprints.
UCLASS(Abstract, Blueprintable)
class UCustomBuffFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunction(UBuff* Buff, const TSubclassOf<UCustomBuffFunction> FunctionClass);
	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunctions(UBuff* Buff, const TArray<TSubclassOf<UCustomBuffFunction>>& FunctionClasses);
	
	virtual void SetupBuffFunction() override { CustomSetup(); }
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override { CustomApply(ApplyEvent); }
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) override { CustomStack(ApplyEvent); }
	virtual void OnRefresh(const FBuffApplyEvent& ApplyEvent) override { CustomRefresh(ApplyEvent); }
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override { CustomRemove(RemoveEvent); }
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

	EBuffRestrictionType RestrictType = EBuffRestrictionType::None;
	FBuffRestriction Restrict;
	UPROPERTY()
	UBuffHandler* TargetComponent = nullptr;

	void SetRestrictionVars(const EBuffRestrictionType RestrictionType, const FBuffRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void BuffRestriction(UBuff* Buff, const EBuffRestrictionType RestrictionType, const FBuffRestriction& Restriction);
};