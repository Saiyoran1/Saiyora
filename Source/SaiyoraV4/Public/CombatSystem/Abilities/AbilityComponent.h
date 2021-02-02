// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "Components/ActorComponent.h"
#include "SaiyoraGameState.h"
#include "AbilityComponent.generated.h"

class UResourceHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	static const float MinimumGlobalCooldownLength;
	static const float MinimumCastLength;
	
	UAbilityComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	void NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility);

	UFUNCTION(BlueprintCallable)
	FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable)
	FCancelEvent CancelCurrentCast();

	UFUNCTION(BlueprintCallable, BlueprintPure)
    UCombatAbility* FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FCastingState GetCastingState() const { return CastingState; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FGlobalCooldown GetGlobalCooldownState() const { return GlobalCooldownState; }

private:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> ActiveAbilities;

	UPROPERTY(ReplicatedUsing = OnRep_GlobalCooldownState)
	FGlobalCooldown GlobalCooldownState;
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal);
	void StartGlobalCooldown(UCombatAbility* Ability);
	void EndGlobalCooldown();

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;
	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);
	void StartCast(UCombatAbility* Ability);
	void CompleteCast();
	void TickCurrentCast();
	void EndCast();
	
	FGlobalCooldownNotification OnGlobalCooldownChanged;
	FAbilityInstanceNotification OnAbilityAdded;
	FAbilityInstanceNotification OnAbilityRemoved;
	FAbilityNotification OnAbilityStart;
	FAbilityNotification OnAbilityTick;
	FAbilityNotification OnAbilityComplete;
	FAbilityCancelNotification OnAbilityCancelled;
	FCastingStateNotification OnCastStateChanged;
	FInterruptNotification OnAbilityInterrupted;

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UPROPERTY()
	UResourceHandler* ResourceHandler;
};