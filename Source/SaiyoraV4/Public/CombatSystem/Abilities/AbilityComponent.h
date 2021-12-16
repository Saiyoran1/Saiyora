﻿#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "GameplayTagsManager.h"
#include "GameFramework/GameState.h"

#include "AbilityComponent.generated.h"

class UCrowdControlHandler;
class UDamageHandler;
class UStatHandler;
class UResourceHandler;
class AGameState;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:

	static FGameplayTag AbilityRestrictionTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("AbilityRestriction")), false); }
	static FGameplayTag AbilityClassRestrictionTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("AbilityRestriction.Class")), false); }
	static FGameplayTag GlobalCooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false); }
	static FGameplayTag CastLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false); }
	static FGameplayTag CooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false); }

	static const float MaxPingCompensation;
	static const float MinimumCastLength;
	static const float MinimumGlobalCooldownLength;
	static const float MinimumCooldownLength;
	static const float AbilityQueWindow;

	UAbilityComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	AGameState* GetGameStateRef() const { return GameStateRef; }
	UResourceHandler* GetResourceHandlerRef() const { return ResourceHandlerRef; }

private:

	bool bLocallyControlled = false;
	UPROPERTY()
	AGameState* GameStateRef = nullptr;
	UPROPERTY()
	UResourceHandler* ResourceHandlerRef = nullptr;
	UPROPERTY()
	UStatHandler* StatHandlerRef = nullptr;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef = nullptr;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandlerRef = nullptr;

//Ability Management

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bIgnoreRestrictions = false);
	void NotifyOfReplicatedAbility(UCombatAbility* NewAbility);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedAbilityRemoval(UCombatAbility* RemovedAbility);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass) const { return ActiveAbilities.FindRef(AbilityClass); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	void GetActiveAbilities(TArray<UCombatAbility*>& OutAbilities) const { ActiveAbilities.GenerateValueArray(OutAbilities); }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityRemoved(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityRemoved(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddAbilityAcquisitionRestriction(FAbilityClassRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbilityAcquisitionRestriction(FAbilityClassRestriction const& Restriction);

private:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TSet<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TMap<TSubclassOf<UCombatAbility>, UCombatAbility*> ActiveAbilities;
	TArray<FAbilityClassRestriction> AbilityAcquisitionRestrictions;
	FAbilityInstanceNotification OnAbilityAdded;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;
	UFUNCTION()
	void CleanupRemovedAbility(UCombatAbility* Ability) { RecentlyRemovedAbilities.Remove(Ability); }
	FAbilityInstanceNotification OnAbilityRemoved;

//Ability Usage

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FAbilityEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool CanUseAbility(UCombatAbility* Ability, ECastFailReason& OutFailReason) const;
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	void AddAbilityTagRestriction(UBuff* Source, FGameplayTag const Tag);
	void RemoveAbilityTagRestriction(UBuff* Source, FGameplayTag const Tag);
	void AddAbilityClassRestriction(UBuff* Source, TSubclassOf<UCombatAbility> const Class);
	void RemoveAbilityClassRestriction(UBuff* Source, TSubclassOf<UCombatAbility> const Class);

private:

	UPROPERTY()
	TMultiMap<FGameplayTag, UBuff*> AbilityUsageTagRestrictions;
	UPROPERTY()
	TMultiMap<TSubclassOf<UCombatAbility>, UBuff*> AbilityUsageClassRestrictions;
	int32 LastPredictionID = 0;
	int32 GenerateNewPredictionID();
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(FAbilityRequest const& Request);
	bool ServerPredictAbility_Validate(FAbilityRequest const& Request) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientPredictionResult(FServerAbilityResult const& Result);
	FTimerHandle TickHandle;
	UFUNCTION()
	void TickCurrentCast();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityTick(FAbilityEvent const& Event);
	TMap<FPredictedTick, bool> PredictedTickRecord;
	TArray<FAbilityEvent> TicksAwaitingParams;
	TMap<FPredictedTick, FCombatParameters> ParamsAwaitingTicks;
	void RemoveExpiredTicks();
	FAbilityNotification OnAbilityTick;
	FAbilityMispredictionNotification OnAbilityMispredicted;

//Cancelling

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityCancelled(FAbilityCancelCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityCancelled(FAbilityCancelCallback const& Callback);
		
private:

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCancelAbility(FCancelRequest const& Request);
	bool ServerCancelAbility_Validate(FCancelRequest const& Request) { return true; }
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityCancel(FCancelEvent const& Event);
	FAbilityCancelNotification OnAbilityCancelled;

//Interrupting

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityInterrupted(FInterruptCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityInterrupted(FInterruptCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddInterruptRestriction(FInterruptRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveInterruptRestriction(FInterruptRestriction const& Restriction);

private:

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityInterrupt(FInterruptEvent const& InterruptEvent);
	UFUNCTION(Client, Reliable)
	void ClientAbilityInterrupt(FInterruptEvent const& InterruptEvent);
	TArray<FInterruptRestriction> InterruptRestrictions;
	FCrowdControlCallback CcCallback;
	UFUNCTION()
	void InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus);
	FLifeStatusCallback DeathCallback;
	UFUNCTION()
	void InterruptCastOnDeath(AActor* Actor, ELifeStatus const PreviousStatus, ELifeStatus const NewStatus);
	FInterruptNotification OnAbilityInterrupted;

//Casting

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool IsCasting() const { return CastingState.bIsCasting; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool IsInterruptible() const { return CastingState.bIsCasting && CastingState.bInterruptible; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCurrentCastLength() const { return CastingState.bIsCasting && CastingState.CastEndTime != 0.0f ? FMath::Max(0.0f, CastingState.CastEndTime - CastingState.CastStartTime) : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCastTimeRemaining() const { return CastingState.bIsCasting && CastingState.CastEndTime != 0.0f ? FMath::Max(0.0f, CastingState.CastEndTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToCastStateChanged(FCastingStateCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromCastStateChanged(FCastingStateCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddCastLengthModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveCastLengthModifier(FAbilityModCondition const& Modifier);

private:

	UPROPERTY(ReplicatedUsing=OnRep_CastingState)
	FCastingState CastingState;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& Previous) { OnCastStateChanged.Broadcast(Previous, CastingState); }
	void StartCast(UCombatAbility* Ability, int32 const PredictionID = 0);
	void EndCast();
	TArray<FAbilityModCondition> CastLengthMods;
	FAbilityModCondition StatCastLengthMod;
	UFUNCTION()
	FCombatModifier ModifyCastLengthFromStat(UCombatAbility* Ability);
	float CalculateCastLength(UCombatAbility* Ability, bool const bWithPingComp);
	FCastingStateNotification OnCastStateChanged;

//Global Cooldown

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool IsGlobalCooldownActive() const { return GlobalCooldownState.bGlobalCooldownActive; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCurrentGlobalCooldownLength() const { return GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime != 0.0f ? FMath::Max(0.0f, GlobalCooldownState.EndTime - GlobalCooldownState.StartTime) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownTimeRemaining() const { return GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime != 0.0f ? FMath::Max(0.0f, GlobalCooldownState.EndTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier);

private:

	FGlobalCooldown GlobalCooldownState;
	void StartGlobalCooldown(UCombatAbility* Ability, int32 const PredictionID = 0);
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void EndGlobalCooldown();
	TArray<FAbilityModCondition> GlobalCooldownMods;
	FAbilityModCondition StatGlobalCooldownMod;
	UFUNCTION()
	FCombatModifier ModifyGlobalCooldownFromStat(UCombatAbility* Ability);
	float CalculateGlobalCooldownLength(UCombatAbility* Ability, bool const bWithPingComp);
	FGlobalCooldownNotification OnGlobalCooldownChanged;

//Cooldown

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveCooldownModifier(FAbilityModCondition const& Modifier);
	float CalculateCooldownLength(UCombatAbility* Ability, bool const bWithPingComp);

private:

	TArray<FAbilityModCondition> CooldownMods;
	FAbilityModCondition StatCooldownMod;
	UFUNCTION()
	FCombatModifier ModifyCooldownFromStat(UCombatAbility* Ability);

//Costs
//TODO: Costs. Need owning client to have accurate cost modifiers, and cost modifiers must behave like ability use restrictions, so no delegates.

public:

	void AddResourceCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier, TSubclassOf<UCombatAbility> const AbilityClass = nullptr);
	void RemoveResourceCostModifier(UObject* Source, TSubclassOf<UResource> const ResourceClass, TSubclassOf<UCombatAbility> const AbilityClass = nullptr);

private:

	//Multimap of AbilityClass to ResourceClass to Mod combo struct.
	//Don't use nullptr as a map key. Use a separate generic multimap.
	//Don't replicate modifiers, just replicate the ability costs themselves.
	TMultiMap<TSubclassOf<UCombatAbility>, FAbilityCostModifiers> ClassResourceCostMods;
	TMultiMap<TSubclassOf<UResource>, FCombatModifier> GenericResourceCostMods;

//Queueing

private:

	EQueueStatus QueueStatus = EQueueStatus::Empty;
	TSubclassOf<UCombatAbility> QueuedAbility;
	bool bUsingAbilityFromQueue = false;
	bool TryQueueAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	FTimerHandle QueueExpirationHandle;
	UFUNCTION()
	void ExpireQueue();
	
};