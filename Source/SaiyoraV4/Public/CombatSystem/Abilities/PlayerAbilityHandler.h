// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "PlayerAbilityHandler.generated.h"

class UPlayerAbilitySave;

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UPlayerAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

	//Player Ability Handler
	
public:
	
	UPlayerAbilityHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
private:
	
	UFUNCTION()
	void UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source);

	static const int32 AbilitiesPerBar;
	FPlayerAbilityLoadout Loadout;
	TSet<TSubclassOf<UCombatAbility>> Spellbook;

	FAbilityBindingNotification OnAbilityBindUpdated;
	FBarSwapNotification OnBarSwap;
	FSpellbookNotification OnSpellbookUpdated;

	EActionBarType CurrentBar = EActionBarType::None;

	UFUNCTION()
	bool CheckForCorrectAbilityPlane(UCombatAbility* Ability);

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AbilityInput(int32 const BindNumber, bool const bHidden);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void LearnAbility(TSubclassOf<UCombatAbility> const NewAbility);
	
	UFUNCTION(BlueprintCallable)
	void SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void SubscribeToBarSwap(FBarSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromBarSwap(FBarSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void SubscribeToSpellbookUpdated(FSpellbookCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromSpellbookUpdated(FSpellbookCallback const& Callback);

	static int32 GetAbilitiesPerBar() { return AbilitiesPerBar; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FPlayerAbilityLoadout GetPlayerLoadout() const { return Loadout; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TSet<TSubclassOf<UCombatAbility>> GetUnlockedAbilities() const { return Spellbook; }
};