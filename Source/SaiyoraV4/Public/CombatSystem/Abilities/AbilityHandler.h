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

UCLASS( ClassGroup=(Custom), Abstract, meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UAbilityHandler : public UActorComponent
{
	GENERATED_BODY()

public:

	static const float MinimumGlobalCooldownLength;
	static const float MinimumCastLength;
	static const float MinimumCooldownLength;
	static FGameplayTag CastLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false); }
	static FGameplayTag GlobalCooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false); }
	static FGameplayTag CooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false); }

//Setup Functions
public:
	UAbilityHandler();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
protected:
	UDamageHandler* GetDamageHandler() const { return DamageHandler; }
	UResourceHandler* GetResourceHandler() const { return ResourceHandler; }
	UStatHandler* GetStatHandler() const { return StatHandler; }
	UCrowdControlHandler* GetCrowdControlHandler() const { return CrowdControlHandler; }
	AGameStateBase* GetGameState() const { return GameStateRef; }
private:
	UPROPERTY()
	AGameStateBase* GameStateRef;
	UPROPERTY()
	UResourceHandler* ResourceHandler;
	UPROPERTY()
	UStatHandler* StatHandler;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandler;
	UPROPERTY()
	UDamageHandler* DamageHandler;
private:
	UFUNCTION()
	void InterruptCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus);
	UFUNCTION()
	void InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus);

//Parent Class: Ability Management
public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility);
	void NotifyOfReplicatedRemovedAbility(UCombatAbility* RemovedAbility);
private:
	UFUNCTION()
	void CleanupRemovedAbility(UCombatAbility* Ability);
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TArray<UCombatAbility*> GetActiveAbilities() const { return ActiveAbilities; }
private:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;

//Parent Class: Mods and Restrictions
public:
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool CheckAbilityRestricted(UCombatAbility* Ability);
private:
	TArray<FAbilityRestriction> AbilityRestrictions;

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddInterruptRestriction(FInterruptRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveInterruptRestriction(FInterruptRestriction const& Restriction);
	bool CheckInterruptRestricted(FInterruptEvent const& InterruptEvent);
private:
	TArray<FInterruptRestriction> InterruptRestrictions;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddCastLengthModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveCastLengthModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float CalculateCastLength(UCombatAbility* Ability);
private:
	UFUNCTION()
	FCombatModifier ModifyCastLengthFromStat(UCombatAbility* Ability);
	TArray<FAbilityModCondition> CastLengthMods;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintPure, Category = "Abilities")
	float CalculateCooldownLength(UCombatAbility* Ability);
private:
	UFUNCTION()
	FCombatModifier ModifyCooldownFromStat(UCombatAbility* Ability);
	TArray<FAbilityModCondition> CooldownLengthMods;

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float CalculateGlobalCooldownLength(UCombatAbility* Ability);
private:
	UFUNCTION()
	FCombatModifier ModifyGlobalCooldownFromStat(UCombatAbility* Ability);
	TArray<FAbilityModCondition> GlobalCooldownMods;

//Parent Class: Subscriptions
public:
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
protected:
	void FireOnAbilityTick(FCastEvent const& CastEvent);
	void FireOnAbilityComplete(UCombatAbility* Ability);
	void FireOnAbilityCancelled(FCancelEvent const& CancelEvent);
	void FireOnAbilityInterrupted(FInterruptEvent const& InterruptEvent);
	void FireOnCastStateChanged(FCastingState const& Previous, FCastingState const& New);
	void FireOnGlobalCooldownChanged(FGlobalCooldown const& Previous, FGlobalCooldown const& New);
private:
	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
	FAbilityNotification OnAbilityTick;
	FAbilityInstanceNotification OnAbilityComplete;
	FAbilityCancelNotification OnAbilityCancelled;
	FInterruptNotification OnAbilityInterrupted;
	FCastingStateNotification OnCastStateChanged;
	FGlobalCooldownNotification OnGlobalCooldownChanged;
	
//Child Class: Getters
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual bool GetIsCasting() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual float GetCurrentCastLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual float GetCastTimeRemaining() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual bool GetIsInterruptible() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual UCombatAbility* GetCurrentCastAbility() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual bool GetGlobalCooldownActive() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual float GetCurrentGlobalCooldownLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual float GetGlobalCooldownTimeRemaining() const;

//Child Class: Functionality
public:
	
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions);

	//End Interface (player and npc)

	//Implementation details (player and npc)
	/*
protected:
	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	void AuthStartGlobal(UCombatAbility* Ability, bool const bPredicted);
	UFUNCTION()
	void EndGlobalCooldown();
	
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
	void AuthStartCast(UCombatAbility* Ability, bool const bPredicted, int32 const PredictionID);
	UFUNCTION()
	void CompleteCast();
	UFUNCTION()
	void TickCurrentAbility();
	void EndCast();
private:
	

protected:

	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityTick(FCastEvent const& CastEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityComplete(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityCancel(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityInterrupt(FInterruptEvent const& InterruptEvent);

	FCastEvent AuthUseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	FCancelEvent AuthCancelAbility();

*/
};