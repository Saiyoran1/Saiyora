// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameplayTagContainer.h"
#include "ResourceStructs.h"
#include "Resource.generated.h"

class UCombatAbility;
class UResourceHandler;
class UStatHandler;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UResource : public UObject
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	FGameplayTag ResourceTag;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMinimum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MinimumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	bool bValueFollowsMinimum = false;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMaximum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MaximumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	bool bValueFollowsMaximum = false;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultValue = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Handler)
	UResourceHandler* Handler = nullptr;
	UFUNCTION()
	void OnRep_Handler();
	void InitializeReplicatedResource();
	bool bInitialized = false;
	UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
	bool bDeactivated = false;
	UFUNCTION()
	void OnRep_Deactivated();

	UPROPERTY(ReplicatedUsing = OnRep_ResourceState)
	FResourceState ResourceState;
	FResourceState PredictedResourceState;
	TMap<int32, float> ResourcePredictions;
	void RecalculatePredictedResource(UObject* ChangeSource);
	void PurgeOldPredictions();
	UFUNCTION()
	void OnRep_ResourceState(FResourceState const& PreviousState);
	bool bReceivedInitialState = false;

	void SetNewMinimum(float const NewValue);
	void SetNewMaximum(float const NewValue);

	UFUNCTION()
	void UpdateMinimumFromStatBind(FGameplayTag const& StatTag, float const NewValue);
	UFUNCTION()
	void UpdateMaximumFromStatBind(FGameplayTag const& StatTag, float const NewValue);

	TArray<FResourceDeltaModifier> ResourceDeltaMods;

	FResourceValueNotification OnResourceChanged;
	FResourceTagNotification OnResourceDeltaModsChanged;

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void InitializeResource();
	UFUNCTION(BlueprintImplementableEvent)
	void DeactivateResource();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	UResourceHandler* GetHandler() const { return Handler; }
	
public:

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AuthInitializeResource(UResourceHandler* NewHandler, UStatHandler* StatHandler, FResourceInitInfo const& InitInfo);
	void AuthDeactivateResource();

	bool CalculateAndCheckAbilityCost(UCombatAbility* Ability, FAbilityCost& Cost);
	void CommitAbilityCost(UCombatAbility* Ability, int32 const PredictionID, FAbilityCost const& Cost);
	void PredictAbilityCost(UCombatAbility* Ability, int32 const PredictionID, FAbilityCost const& Cost);
	void RollbackFailedCost(int32 const PredictionID);
	void UpdateCostPredictionFromServer(int32 const PredictionID, FAbilityCost const& ServerCost);

	void AddResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	void RemoveResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	
	void SubscribeToResourceChanged(FResourceValueCallback const& Callback);
	void UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback);
	void SubscribeToResourceModsChanged(FResourceTagCallback const& Callback);
	void UnsubscribeFromResourceModsChanged(FResourceTagCallback const& Callback);

	FGameplayTag GetResourceTag() const { return ResourceTag; }
	float GetCurrentValue() const;
	float GetMinimum() const { return ResourceState.Minimum; }
	float GetMaximum() const { return ResourceState.Maximum; }
	bool GetInitialized() const { return bInitialized; }
	bool GetDeactivated() const { return bDeactivated; }
};