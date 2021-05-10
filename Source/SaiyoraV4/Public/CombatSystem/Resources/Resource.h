// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceStructs.h"
#include "Styling/SlateBrush.h"
#include "Resource.generated.h"

class UCombatAbility;
class UResourceHandler;
class UStatHandler;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UResource : public UObject
{
	GENERATED_BODY()

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
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	FSlateBrush BarFillBrush;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	FSlateBrush BarBackgroundBrush;

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
	void SetResourceValue(float const NewValue, UObject* Source, int32 const PredictionID = 0);

	UFUNCTION()
	void UpdateMinimumFromStatBind(FGameplayTag const& StatTag, float const NewValue);
	UFUNCTION()
	void UpdateMaximumFromStatBind(FGameplayTag const& StatTag, float const NewValue);

	TArray<FResourceDeltaModifier> ResourceDeltaMods;

	FResourceValueNotification OnResourceChanged;
	FResourceInstanceNotification OnResourceDeltaModsChanged;

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
	virtual UWorld* GetWorld() const override;
	
	void AuthInitializeResource(UResourceHandler* NewHandler, UStatHandler* StatHandler, FResourceInitInfo const& InitInfo);
	void AuthDeactivateResource();

	bool CalculateAndCheckAbilityCost(UCombatAbility* Ability, float& Cost, bool const bStaticCost);
	void CommitAbilityCost(UCombatAbility* Ability, int32 const PredictionID, float const Cost);
	void PredictAbilityCost(UCombatAbility* Ability, int32 const PredictionID, float const Cost);
	void RollbackFailedCost(int32 const PredictionID);
	void UpdateCostPredictionFromServer(int32 const PredictionID, float const ServerCost);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void ModifyResource(UObject* Source, float const Amount, bool const bIgnoreModifiers);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void AddResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void RemoveResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceChanged(FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceModsChanged(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceModsChanged(FResourceInstanceCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetCurrentValue() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMinimum() const { return ResourceState.Minimum; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMaximum() const { return ResourceState.Maximum; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	FSlateBrush GetDisplayFillBrush() const { return BarFillBrush; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	FSlateBrush GetDisplayBackgroundBrush() const { return BarBackgroundBrush; }
	
	bool GetInitialized() const { return bInitialized; }
	bool GetDeactivated() const { return bDeactivated; }
};