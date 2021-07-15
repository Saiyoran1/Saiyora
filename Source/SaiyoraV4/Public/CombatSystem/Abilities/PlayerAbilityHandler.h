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
	
public:
	
	UPlayerAbilityHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AbilityInput(int32 const BindNumber, bool const bHidden);
	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void LearnAbility(TSubclassOf<UCombatAbility> const NewAbility);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UpdateAbilityBind(TSubclassOf<UCombatAbility> const Ability, int32 const Bind, EActionBarType const Bar);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
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

private:

	FCastEvent PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bFromQueue = false);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(FAbilityRequest const& AbilityRequest);
	bool ServerPredictAbility_Validate(FAbilityRequest const& AbilityRequest) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientSucceedPredictedAbility(FServerAbilityResult const& ServerResult);
	UFUNCTION(Client, Reliable)
	void ClientFailPredictedAbility(int32 const PredictionID, ECastFailReason const FailReason);

	int32 GenerateNewPredictionID();
	static int32 ClientPredictionID = 0;
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;

	void StartGlobal(UCombatAbility* Ability, bool const bPredicted = false);\
	virtual void EndGlobalCooldown() override;
	void PredictStartGlobal(int32 const PredictionID);
	void UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult);

	void StartCast(UCombatAbility* Ability, int32 const PredictionID = 0);
	void PredictStartCast(UCombatAbility* Ability, int32 const PredictionID);
	void UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult);
	virtual void CompleteCast() override;
	
	bool TryQueueAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION()
	void ClearQueue();
	void SetQueueExpirationTimer();
	UPROPERTY()
	TSubclassOf<UCombatAbility> QueuedAbility;
	EQueueStatus QueueStatus = EQueueStatus::Empty;
	FTimerHandle QueueClearHandle;
	void CheckForQueuedAbilityOnGlobalEnd();
	void CheckForQueuedAbilityOnCastEnd();

	UFUNCTION()
	void SwapBarOnPlaneSwap(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source);

	static const int32 AbilitiesPerBar;
	FPlayerAbilityLoadout Loadout;
	TSet<TSubclassOf<UCombatAbility>> Spellbook;

	FAbilityBindingNotification OnAbilityBindUpdated;
	FBarSwapNotification OnBarSwap;
	FSpellbookNotification OnSpellbookUpdated;

	EActionBarType CurrentBar = EActionBarType::None;

	UFUNCTION()
	bool CheckForCorrectAbilityPlane(UCombatAbility* Ability);

	EAbilityPermission AbilityPermission = EAbilityPermission::None;
};