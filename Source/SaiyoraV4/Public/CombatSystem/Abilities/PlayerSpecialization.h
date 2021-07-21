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
	EActionBarType PrimaryBar = EActionBarType::None;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TSet<TSubclassOf<UCombatAbility>> GrantedAbilities;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TMap<TSubclassOf<UCombatAbility>, FAbilityTalentInfo> TalentInformation;

	UPROPERTY()
	TMap<UCombatAbility*, int32> SelectedTalents;
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
	void UnlearnSpecObject();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Specialization")
	EActionBarType GetGrantedAbilities(TSet<TSubclassOf<UCombatAbility>>& OutAbilities) const;
	UFUNCTION(BlueprintCallable, Category = "Specialization")
	bool GetTalentInfo(TSubclassOf<UCombatAbility> const AbilityClass, FAbilityTalentInfo& OutInfo) const;
};