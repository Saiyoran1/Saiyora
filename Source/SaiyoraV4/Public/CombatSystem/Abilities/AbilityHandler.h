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

	friend struct FAbilityValues;
	friend struct FAbilityResourceCost;

public:
	static const float MinimumGlobalCooldownLength;
	static const float MinimumCastLength;
	static const float MinimumCooldownLength;
	static FGameplayTag CastLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false); }
	static FGameplayTag GlobalCooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false); }
	static FGameplayTag CooldownLengthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false); }
//Setup
	UAbilityHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	AGameState* GetGameStateRef() const { return GameStateRef; }
	UResourceHandler* GetResourceHandlerRef() const { return ResourceHandler; }
	UStatHandler* GetStatHandlerRef() const { return StatHandler; }
	UCrowdControlHandler* GetCrowdControlHandlerRef() const { return CrowdControlHandler; }
	UDamageHandler* GetDamageHandlerRef() const { return DamageHandler; }
private:
	UPROPERTY()
	AGameState* GameStateRef;
	UPROPERTY()
	UResourceHandler* ResourceHandler;
	UPROPERTY()
	UStatHandler* StatHandler;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandler;
	UPROPERTY()
	UDamageHandler* DamageHandler;
//Ability Management
public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	virtual UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedRemovedAbility(UCombatAbility* RemovedAbility);
	UFUNCTION()
	void CleanupRemovedAbility(UCombatAbility* Ability);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TArray<UCombatAbility*> GetActiveAbilities() const { return ActiveAbilities; }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityAdded(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityRemoved(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityRemoved(FAbilityInstanceCallback const& Callback);
private:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;
	virtual void SetupInitialAbilities();
	TMap<TSubclassOf<UCombatAbility>, FAbilityValues> ClassSpecificModifiers;
protected:
	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
//Ability Usage
public:
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbilityRestriction(FAbilityRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool CheckCanUseAbility(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float>& OutCosts, ECastFailReason& OutFailReason);
private:
	TArray<FAbilityRestriction> AbilityRestrictions;
	bool CheckAbilityRestricted(UCombatAbility* Ability);
protected:
	UPROPERTY(BlueprintReadWrite, Category = "Abilities")
	FCombatParameters AbilityTickBroadcastParams;
//Casting
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCastLength(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FCastingState GetCastingState() const { return CastingState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool GetIsCasting() const { return CastingState.bIsCasting; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCurrentCastLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCastTimeRemaining() const;
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToCastStateChanged(FCastingStateCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromCastStateChanged(FCastingStateCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityCompleted(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityCompleted(FAbilityInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddConditionalCastLengthModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveConditionalCastLengthModifier(int32 const ModifierID);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassCastLengthModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abiltiies")
	void RemoveClassCastLengthModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
protected:
	FCastingStateNotification OnCastStateChanged;
	FAbilityNotification OnAbilityTick;
	FAbilityInstanceNotification OnAbilityComplete;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
	void StartCast(UCombatAbility* Ability);
	UFUNCTION()
	void TickCurrentAbility();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityTick(FCastEvent const& CastEvent, FCombatParameters const& BroadcastParams);
	UFUNCTION()
	virtual void CompleteCast();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityComplete(FCastEvent const& CastEvent);
	void EndCast();
private:
	TMap<int32, FAbilityModCondition> ConditionalCastLengthModifiers;
	UFUNCTION()
	FCombatModifier ModifyCastLengthFromStat(TSubclassOf<UCombatAbility> AbilityClass);
//Cancelling
public:
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityCancelled(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityCancelled(FAbilityCallback const& Callback);
protected:
	FAbilityCancelNotification OnAbilityCancelled;
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityCancel(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams);
//Interrupting
public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityInterrupted(FInterruptCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityInterrupted(FInterruptCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void AddInterruptRestriction(FInterruptRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveInterruptRestriction(FInterruptRestriction const& Restriction);
protected:
	FInterruptNotification OnAbilityInterrupted;
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityInterrupt(FInterruptEvent const& InterruptEvent);
private:
	TArray<FInterruptRestriction> InterruptRestrictions;
	bool CheckInterruptRestricted(FInterruptEvent const& InterruptEvent);
	UFUNCTION()
	void InterruptCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus);
	UFUNCTION()
	void InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus);
//Global Cooldown
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownLength(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FGlobalCooldown GetGlobalCooldownState() const { return GlobalCooldownState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool GetIsGlobalCooldownActive() const { return GlobalCooldownState.bGlobalCooldownActive; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCurrentGlobalCooldownLength() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownTimeRemaining() const;
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddConditionalGlobalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveConditionalGlobalCooldownModifier(int32 const ModifierID);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassGlobalCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveClassGlobalCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
protected:
	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	FGlobalCooldownNotification OnGlobalCooldownChanged;
	void StartGlobal(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION()
	virtual void EndGlobalCooldown();
private:
	TMap<int32, FAbilityModCondition> ConditionalGlobalCooldownModifiers;
	UFUNCTION()
	FCombatModifier ModifyGlobalCooldownFromStat(TSubclassOf<UCombatAbility> AbilityClass);
//Cooldown
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	float GetCooldownLength(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddConditionalCooldownModifier(FAbilityModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveConditionalCooldownModifier(int32 const ModifierID);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveClassCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	int32 GetMaxCharges(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassMaxChargeModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveClassMaxChargeModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	int32 GetChargeCost(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassChargeCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveClassChargeCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	int32 GetChargesPerCooldown(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassChargesPerCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abiltiies")
	void RemoveClassChargesPerCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID);
private:
	TMap<int32, FAbilityModCondition> ConditionalCooldownModifiers;
	UFUNCTION()
	FCombatModifier ModifyCooldownFromStat(TSubclassOf<UCombatAbility> AbilityClass);
//Cost
public:
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddConditionalCostModifier(FAbilityResourceModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveConditionalCostModifier(int32 const ModifierID);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 AddClassAbilityCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveClassAbilityCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, TSubclassOf<UResource> const ResourceClass, int32 const ModifierID);
private:
	TMap<int32, FAbilityResourceModCondition> ConditionalCostModifiers;
};