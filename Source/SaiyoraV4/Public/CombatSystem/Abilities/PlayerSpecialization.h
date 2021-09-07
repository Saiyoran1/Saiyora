// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerAbilityHandler.h"
#include "PlayerSpecialization.generated.h"

class UCombatAbility;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UPlayerSpecialization : public UObject
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	EActionBarType SpecBar = EActionBarType::Ancient;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TSet<TSubclassOf<UPlayerCombatAbility>> DefaultAbilities;
	UPROPERTY()
	UPlayerAbilityHandler* Handler = nullptr;
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Specialization")
	void OnSpecInitialize();
	UFUNCTION(BlueprintImplementableEvent, Category = "Specialization")
	void OnSpecUnlearned();
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	EActionBarType GetSpecBar() const { return SpecBar; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	void GetDefaultAbilities(TSet<TSubclassOf<UPlayerCombatAbility>>& OutAbilities) const { OutAbilities = DefaultAbilities; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	UPlayerAbilityHandler* GetHandler() const { return Handler; }
	void InitializeSpecObject(UPlayerAbilityHandler* NewHandler);
	void UnlearnSpecObject();
};