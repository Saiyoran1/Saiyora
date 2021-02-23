// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "BuffFunction.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UBuffFunction : public UObject
{
	GENERATED_BODY()

private:

	UPROPERTY()
	UBuff* OwningBuff = nullptr;
	bool bBuffFunctionInitialized = false;
	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	FGameplayTagContainer FunctionTags;

public:

	virtual void SetupBuffFunction(UBuff* BuffRef);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* GetOwningBuff() const { return OwningBuff; }
	FGameplayTagContainer const& GetBuffFunctionTags() const { return FunctionTags; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Buff")
	void OnApply(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, Category = "Buff")
	void OnStack(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, Category = "Buff")
	void OnRefresh(FBuffApplyEvent const& ApplyEvent);
	UFUNCTION(BlueprintImplementableEvent, Category = "Buff")
	void OnRemove(FBuffRemoveEvent const& RemoveEvent);

protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "Buff")
	void InitializeBuffFunction();
};