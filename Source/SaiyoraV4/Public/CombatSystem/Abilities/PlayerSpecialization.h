// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "PlayerAbilityHandler.h"
#include "UObject/NoExportTypes.h"
#include "PlayerSpecialization.generated.h"

struct FAbilityTalentInfo;
class UCombatAbility;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UPlayerSpecialization : public UObject
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TMap<TSubclassOf<UCombatAbility>, FAbilityTalentInfo> SpecAbilities;

	UPROPERTY()
	TMap<TSubclassOf<UCombatAbility>, TSubclassOf<UCombatAbility>> SelectedTalents;
	UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
	UPlayerAbilityHandler* OwningComponent;
	UFUNCTION()
	void OnRep_OwningComponent();
	bool bInitialized = false;
	
protected:
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetupSpecObject();
	UFUNCTION(BlueprintImplementableEvent)
	void DeactivateSpecObject();
	
public:
	
	void InitializeSpecObject(UPlayerAbilityHandler* AbilityHandler);
	void InitializeSpecObject(UPlayerAbilityHandler* AbilityHandler, TMap<TSubclassOf<UCombatAbility>, TSubclassOf<UCombatAbility>> const& TalentSetup);
	void UnlearnSpecObject();
	void CreateNewDefaultLoadout(FPlayerAbilityLoadout& OutLoadout);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	void GetGrantedAbilities(TArray<TSubclassOf<UCombatAbility>>& GrantedAbilities) const { SelectedTalents.GenerateValueArray(GrantedAbilities); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	void GetSpecBaseAbilities(TArray<TSubclassOf<UCombatAbility>>& BaselineAbilities) const { SpecAbilities.GenerateKeyArray(BaselineAbilities); }
	UFUNCTION(BlueprintCallable, Category = "Specialization")
	bool GetTalentInfo(TSubclassOf<UCombatAbility> const AbilityClass, FAbilityTalentInfo& OutInfo) const;
};