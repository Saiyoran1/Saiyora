// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "Components/ActorComponent.h"
#include "SaiyoraGameState.h"
#include "AbilityHandler.generated.h"

class UStatHandler;
class UResourceHandler;

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
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FCancelEvent CancelCurrentCast();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual FCastingState GetCastingState() const { return CastingState; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	virtual FGlobalCooldown GetGlobalCooldownState() const { return GlobalCooldownState; }

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
	void SubscribeToAbilityStarted(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityStarted(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToAbilityCompleted(FAbilityCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromAbilityCompleted(FAbilityCallback const& Callback);
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
	
private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;

	UPROPERTY(ReplicatedUsing = OnRep_GlobalCooldownState)
	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal);
	void StartGlobalCooldown(UCombatAbility* Ability, int32 const CastID);
	UFUNCTION()
	void EndGlobalCooldown();
	float CalculateGlobalCooldownLength(UCombatAbility* Ability);
	TArray<FAbilityModCondition> GlobalCooldownMods;

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
	void StartCast(UCombatAbility* Ability, int32 const CastID);
	UFUNCTION()
	void CompleteCast();
	UFUNCTION()
	void TickCurrentCast();
	void EndCast();
	float CalculateCastLength(UCombatAbility* Ability);
	TArray<FAbilityModCondition> CastLengthMods;

	TArray<FAbilityModCondition> CooldownLengthMods;

	virtual void GenerateNewCastID(FCastEvent& CastEvent);
	
	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
	FAbilityNotification OnAbilityStart;
	FAbilityNotification OnAbilityTick;
	FAbilityNotification OnAbilityComplete;
	FAbilityCancelNotification OnAbilityCancelled;
	FInterruptNotification OnAbilityInterrupted;
	FCastingStateNotification OnCastStateChanged;
	FGlobalCooldownNotification OnGlobalCooldownChanged;

	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityStart(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityTick(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityComplete(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityCancel(FCancelEvent const& CancelEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityInterrupt(FInterruptEvent const& InterruptEvent);

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	UResourceHandler* ResourceHandler;
	UPROPERTY()
	UStatHandler* StatHandler;
};