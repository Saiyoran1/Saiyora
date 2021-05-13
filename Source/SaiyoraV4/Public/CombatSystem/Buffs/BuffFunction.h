// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "BuffFunction.generated.h"

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

	virtual void SetupBuffFunction() { return; }
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) { return; }
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) { return; }
	virtual void CleanupBuffFunction() { return; }
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
	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override { CustomApply(); }
	virtual void OnStack(FBuffApplyEvent const& ApplyEvent) override { CustomStack(); }
	virtual void OnRefresh(FBuffApplyEvent const& ApplyEvent) override { CustomRefresh(); }
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override { CustomRemove(); }
	virtual void CleanupBuffFunction() override { CustomCleanup(); }

protected:

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Setup"))
	void CustomSetup();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Apply"))
	void CustomApply();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Stack"))
	void CustomStack();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Refresh"))
	void CustomRefresh();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Remove"))
	void CustomRemove();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Cleanup"))
	void CustomCleanup();
};