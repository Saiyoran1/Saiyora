#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"

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

	UAbilityComponent();
	virtual void BeginPlay() override;

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

	FAbilityModCondition StatCooldownMod;
	FAbilityModCondition StatGlobalCooldownMod;
	FAbilityModCondition StatCastLengthMod;
	FCrowdControlCallback CcCallback;
	FLifeStatusCallback DeathCallback;

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
	bool CanUseAbility(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float>& OutCosts, ECastFailReason& OutFailReason) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (HidePin="Source", DefaultToSelf="Source"))
	void AddAbilityTagRestriction(UObject* Source, FGameplayTag const Tag);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (HidePin="Source", DefaultToSelf="Source"))
	void RemoveAbilityTagRestriction(UObject* Source, FGameplayTag const Tag);

private:

	UPROPERTY()
	TMultiMap<UObject*, FGameplayTag> AbilityUsageTagRestrictions;
	UPROPERTY()
	TMultiMap<UObject*, TSubclassOf<UCombatAbility>> AbilityUsageClassRestrictions;

//Casting

public:

private:

	FCastingState CastingState;

//Global Cooldown

public:

private:

	FGlobalCooldown GlobalCooldownState;
};