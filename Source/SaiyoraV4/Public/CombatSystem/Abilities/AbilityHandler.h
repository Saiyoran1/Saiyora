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
	static const float MinimumCooldownLength;
	static const float AbilityQueWindowSec;
	static const float MaxPingCompensation;
	static FGameplayTag CastLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false); }
	static FGameplayTag GlobalCooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false); }
	static FGameplayTag CooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false); }
	
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
	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TArray<UCombatAbility*> GetActiveAbilities() const { return ActiveAbilities; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FCastingState GetCastingState() const { return CastingState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool GetIsCasting() const { return CastingState.bIsCasting; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCurrentCastLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCastTimeRemaining() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FGlobalCooldown GetGlobalCooldownState() const { return GlobalCooldownState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool GetIsGlobalCooldownActive() const { return GlobalCooldownState.bGlobalCooldownActive; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCurrentGlobalCooldownLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownTimeRemaining() const;

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

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float CalculateAbilityCooldown(UCombatAbility* Ability);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbilityRestriction(FAbilityRestriction const& Restriction);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddInterruptRestriction(FInterruptRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveInterruptRestriction(FInterruptRestriction const& Restriction);
	
private:
	
	bool CheckInterruptRestricted(FInterruptEvent const& InterruptEvent);
	TArray<FInterruptRestriction> InterruptRestrictions;
	
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;
protected:
	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	void StartGlobal(UCombatAbility* Ability);
	UFUNCTION()
	virtual void EndGlobalCooldown();
	float CalculateGlobalCooldownLength(UCombatAbility* Ability);
private:
	TArray<FAbilityModCondition> GlobalCooldownMods;
	UFUNCTION()
	FCombatModifier ModifyGlobalCooldownFromStat(UCombatAbility* Ability);
protected:
	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
private:
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
protected:
	void StartCast(UCombatAbility* Ability);
	UFUNCTION()
	virtual void CompleteCast();
	UFUNCTION()
	void TickCurrentAbility();
	void EndCast();
	float CalculateCastLength(UCombatAbility* Ability);
private:
	TArray<FAbilityModCondition> CastLengthMods;
	UFUNCTION()
	FCombatModifier ModifyCastLengthFromStat(UCombatAbility* Ability);

	TArray<FAbilityModCondition> CooldownLengthMods;
	UFUNCTION()
	FCombatModifier ModifyCooldownFromStat(UCombatAbility* Ability);

	UFUNCTION()
	void InterruptCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus);
	UFUNCTION()
	void InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus);
protected:
	bool CheckCanCastAbility(UCombatAbility* Ability, TArray<FAbilityCost>& OutCosts, ECastFailReason& OutFailReason);
private:
	bool CheckAbilityRestricted(UCombatAbility* Ability);
	TArray<FAbilityRestriction> AbilityRestrictions;
protected:
	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
	FAbilityNotification OnAbilityTick;
	FAbilityInstanceNotification OnAbilityComplete;
	FAbilityCancelNotification OnAbilityCancelled;
	FInterruptNotification OnAbilityInterrupted;
	FCastingStateNotification OnCastStateChanged;
	FGlobalCooldownNotification OnGlobalCooldownChanged;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityTick(FCastEvent const& CastEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityComplete(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityCancel(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityInterrupt(FInterruptEvent const& InterruptEvent);
	
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