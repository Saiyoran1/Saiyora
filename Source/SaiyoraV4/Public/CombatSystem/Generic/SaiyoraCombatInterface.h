// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaiyoraCombatInterface.generated.h"

class UDamageHandler;
class UBuffHandler;
class UAbilityHandler;
class UCrowdControlHandler;
class UResourceHandler;
class UStatHandler;

UINTERFACE(Blueprintable)
class USaiyoraCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class SAIYORAV4_API ISaiyoraCombatInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	UDamageHandler* GetDamageHandler() const;
	virtual UDamageHandler* GetDamageHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Buffs")
	UBuffHandler* GetBuffHandler() const;
	virtual UBuffHandler* GetBuffHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	UAbilityHandler* GetAbilityHandler() const;
	virtual UAbilityHandler* GetAbilityHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crowd Control")
	UCrowdControlHandler* GetCrowdControlHandler() const;
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
	UResourceHandler* GetResourceHandler() const;
	virtual UResourceHandler* GetResourceHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stats")
	UStatHandler* GetStatHandler() const;
	virtual UStatHandler* GetStatHandler_Implementation() const { return nullptr; }
};
