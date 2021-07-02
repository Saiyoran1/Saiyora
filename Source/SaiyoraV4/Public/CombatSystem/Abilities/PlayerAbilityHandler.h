// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "PlayerAbilityHandler.generated.h"

class UPlayerAbilitySave;

UCLASS()
class SAIYORAV4_API UPlayerAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

<<<<<<< HEAD
	//Player Ability Handler
	
public:
	
	static const float AbilityQueWindowSec;
	static const float MaxPingCompensation;

	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass) override;
	virtual FCancelEvent CancelCurrentCast() override;
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions) override;
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	
private:
	
	FCastEvent PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bFromQueue);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(FAbilityRequest const& AbilityRequest);
	bool ServerPredictAbility_Validate(FAbilityRequest const& AbilityRequest) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientSucceedPredictedAbility(FServerAbilityResult const& ServerResult);
	UFUNCTION(Client, Reliable)
	void ClientFailPredictedAbility(int32 const PredictionID, FString const& FailReason);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerHandlePredictedTick(FAbilityRequest const& TickRequest);
	bool ServerHandlePredictedTick_Validate(FAbilityRequest const& TickRequest) { return true; }

	UFUNCTION(Client, Reliable)
	void ClientInterruptCast(FInterruptEvent const& InterruptEvent);
	FCancelEvent PredictCancelAbility();
	UFUNCTION(Server, WithValidation, Reliable)
	void ServerPredictCancelAbility(FCancelRequest const& CancelRequest);
	bool ServerPredictCancelAbility_Validate(FCancelRequest const& CancelRequest) { return true; }

	
	void UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult);

	
	void UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult);
	UFUNCTION()
	void PredictAbilityTick();
	void PurgeExpiredPredictedTicks();
	UFUNCTION()
	void AuthTickPredictedCast();
	void HandleMissedPredictedTick(int32 const TickNumber);
	TArray<FPredictedTick> TicksAwaitingParams;
	TMap<FPredictedTick, FCombatParameters> ParamsAwaitingTicks;

	int32 GenerateNewPredictionID();
	int32 ClientPredictionID = 0;
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;
	FAbilityMispredictionNotification OnAbilityMispredicted;

	bool TryQueueAbility(TSubclassOf<UCombatAbility> const Ability);
	UFUNCTION()
	void ClearQueue();
	void SetQueueExpirationTimer();
	UPROPERTY()
	TSubclassOf<UCombatAbility> QueuedAbility;
	EQueueStatus QueueStatus = EQueueStatus::Empty;
	FTimerHandle QueueClearHandle;
	void CheckForQueuedAbilityOnGlobalEnd();
	void CheckForQueuedAbilityOnCastEnd();

private:
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	bool bOwnerIsLocal = false;

	void PredictStartGlobal(int32 const PredictionID);
	void StartGlobal(UCombatAbility* Ability);
	UFUNCTION()
	void EndGlobalCooldown();
	FPredictedGlobalCooldown GlobalCooldown;
	FTimerHandle GlobalCooldownHandle;

	void PredictStartCast(UCombatAbility* Ability, int32 const PredictionID);
	void StartCast(UCombatAbility* Ability, int32 const PredictionID);
	void EndCast();
	UFUNCTION()
	void CompleteCast();
	UFUNCTION()
	void TickCast();
	FPredictedCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityTick(FCastEvent const& TickEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityComplete(FCastEvent const& CompletionEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityCancel(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityInterrupt(FInterruptEvent const& InterruptEvent);

	//Other

=======
>>>>>>> parent of 56ba23a (Refactor of Ability Component)
	UPlayerAbilityHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source);

	static const int32 AbilitiesPerBar;
	FPlayerAbilityLoadout Loadout;
	TSet<TSubclassOf<UCombatAbility>> Spellbook;

	FAbilityBindingNotification OnAbilityBindUpdated;
	FBarSwapNotification OnBarSwap;

	EActionBarType CurrentBar = EActionBarType::None;
	
	UPROPERTY()
	APlayerState* PlayerStateRef;
	UPROPERTY()
	UPlayerAbilitySave* AbilitySave;

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

	static int32 GetAbilitiesPerBar() { return AbilitiesPerBar; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FPlayerAbilityLoadout GetPlayerLoadout() const { return Loadout; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TSet<TSubclassOf<UCombatAbility>> GetUnlockedAbilities() const { return Spellbook; }
};