// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "Components/ActorComponent.h"
#include "SaiyoraGameState.h"
#include "DamageEnums.h"
#include "CrowdControlStructs.h"
#include "AbilityHandler.generated.h"

class UStatHandler;
class UResourceHandler;
class UCrowdControlHandler;
class UDamageHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UAbilityHandler : public UActorComponent
{
	GENERATED_BODY()

public:

	static const float MinimumGlobalCooldownLength;
	static const float MinimumCastLength;
	static const FGameplayTag CastLengthStatTag;
	static const FGameplayTag GlobalCooldownLengthStatTag;
	static const FGameplayTag CooldownLengthStatTag;
	static const float MinimumCooldownLength;
	static const float AbilityQueWindowSec;
	
	UAbilityHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedRemovedAbility(UCombatAbility* RemovedAbility);
	UFUNCTION()
	void CleanupRemovedAbility(UCombatAbility* Ability);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FCastingState GetCastingState() const { return CastingState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FGlobalCooldown GetGlobalCooldownState() const { return GlobalCooldownState; }

	UResourceHandler* GetResourceHandlerRef() const { return ResourceHandler; }
	UStatHandler* GetStatHandlerRef() const { return StatHandler; }
	ASaiyoraGameState* GetGameStateRef() const { return GameStateRef; }

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityRemoved(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityRemoved(FAbilityInstanceCallback const& Callback);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToAbilityCompleted(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromAbilityCompleted(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToAbilityCancelled(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromAbilityCancelled(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToAbilityInterrupted(FInterruptCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromAbilityInterrupted(FInterruptCallback const& Callback);
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToCastStateChanged(FCastingStateCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromCastStateChanged(FCastingStateCallback const& Callback);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddCastLengthModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveCastLengthModifier(FAbilityModCondition const& Modifier);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintPure, Category = "Abilities")
	float CalculateAbilityCooldown(UCombatAbility* Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbilityRestriction(FAbilityRestriction const& Restriction);
	
private:

	FCastEvent AuthUseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	FCastEvent PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bFromQueue);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(FAbilityRequest const& AbilityRequest);
	bool ServerPredictAbility_Validate(FAbilityRequest const& AbilityRequest) { return true; }
	UFUNCTION(Client, Reliable)
    void ClientSucceedPredictedAbility(FServerAbilityResult const& ServerResult);
	UFUNCTION(Client, Reliable)
	void ClientFailPredictedAbility(int32 const PredictionID);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerHandlePredictedTick(FAbilityRequest const& TickRequest);
	bool ServerHandlePredictedTick_Validate(FAbilityRequest const& TickRequest) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientInterruptCast(FInterruptEvent const& InterruptEvent);
	UFUNCTION(Client, Reliable)
	void ClientCancelCast(FCancelEvent const& CancelEvent);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;

	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	void AuthStartGlobal(UCombatAbility* Ability);
	void PredictStartGlobal(int32 const PredictionID);
	void UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult);
	UFUNCTION()
	void EndGlobalCooldown();
	float CalculateGlobalCooldownLength(UCombatAbility* Ability);
	TArray<FAbilityModCondition> GlobalCooldownMods;
	UFUNCTION()
	FCombatModifier ModifyGlobalCooldownFromStat(UCombatAbility* Ability);

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
	void AuthStartCast(UCombatAbility* Ability, bool const bPredicted, int32 const PredictionID);
	void PredictStartCast(UCombatAbility* Ability, int32 const PredictionID);
	void UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult);
	UFUNCTION()
	void CompleteCast();
	UFUNCTION()
	void TickCurrentAbility();
	UFUNCTION()
    void PredictAbilityTick();
	void PurgeExpiredPredictedTicks();
	UFUNCTION()
	void AuthTickPredictedCast();
	void HandleMissedPredictedTick(int32 const TickNumber);
	TArray<FPredictedTick> TicksAwaitingParams;
	TMap<FPredictedTick, FCombatParameters> ParamsAwaitingTicks;
	void EndCast();
	float CalculateCastLength(UCombatAbility* Ability);
	TArray<FAbilityModCondition> CastLengthMods;
	UFUNCTION()
	FCombatModifier ModifyCastLengthFromStat(UCombatAbility* Ability);

	TArray<FAbilityModCondition> CooldownLengthMods;
	UFUNCTION()
	FCombatModifier ModifyCooldownFromStat(UCombatAbility* Ability);

	UFUNCTION()
	void CancelCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus);
	UFUNCTION()
	void CancelCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus);

	bool CheckAbilityRestricted(UCombatAbility* Ability);
	TArray<FAbilityRestriction> AbilityRestrictions;

	void GenerateNewPredictionID(FCastEvent& CastEvent);
	int32 ClientPredictionID = 0;
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;

	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
	FAbilityNotification OnAbilityTick;
	FAbilityInstanceNotification OnAbilityComplete;
	FAbilityCancelNotification OnAbilityCancelled;
	FInterruptNotification OnAbilityInterrupted;
	FCastingStateNotification OnCastStateChanged;
	FGlobalCooldownNotification OnGlobalCooldownChanged;

	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityTick(FCastEvent const& CastEvent, bool const OwnerNeeds, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityComplete(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityCancel(FCancelEvent const& CancelEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityInterrupt(FInterruptEvent const& InterruptEvent);

	void TryQueueAbility(TSubclassOf<UCombatAbility> const Ability);
	UFUNCTION()
	void ClearQueue();
	void SetQueueExpirationTimer();
	UPROPERTY()
	TSubclassOf<UCombatAbility> QueuedAbility;
	EQueueStatus QueueStatus = EQueueStatus::Empty;
	FTimerHandle QueueClearHandle;
	UFUNCTION()
	void CheckForQueuedAbilityOnGlobalEnd(FGlobalCooldown const& OldGlobalCooldown, FGlobalCooldown const& NewGlobalCooldown);
	UFUNCTION()
	void CheckForQueuedAbilityOnCastEnd(FCastingState const& OldState, FCastingState const& NewState);

protected:
	
	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	UResourceHandler* ResourceHandler;
	UPROPERTY()
	UStatHandler* StatHandler;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandler;
	UPROPERTY()
	UDamageHandler* DamageHandler;
};