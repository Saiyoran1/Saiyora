// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameplayTagContainer.h"
#include "ResourceStructs.h"
#include "Resource.generated.h"

class UCombatAbility;
class UResourceHandler;

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
	virtual void InitializeReplicatedResource();
	bool bInitialized = false;
	UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
	bool bDeactivated = false;
	UFUNCTION()
	void OnRep_Deactivated();

	UPROPERTY(ReplicatedUsing = OnRep_ResourceState)
	FResourceState ResourceState;
	UFUNCTION()
	virtual void OnRep_ResourceState(FResourceState const& PreviousState);
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
	
public:

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AuthInitializeResource(UResourceHandler* NewHandler, FResourceInitInfo const& InitInfo);
	void AuthDeactivateResource();

	bool CalculateAndCheckAbilityCost(UCombatAbility* Ability, FAbilityCost& Cost);
	void CommitAbilityCost(UCombatAbility* Ability, int32 const CastID, FAbilityCost const& Cost);

	void AddResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	void RemoveResourceDeltaModifier(FResourceDeltaModifier const& Modifier);
	
	void SubscribeToResourceChanged(FResourceValueCallback const& Callback);
	void UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback);
	void SubscribeToResourceModsChanged(FResourceTagCallback const& Callback);
	void UnsubscribeFromResourceModsChanged(FResourceTagCallback const& Callback);

	FGameplayTag GetResourceTag() const { return ResourceTag; }
	virtual float GetCurrentValue() const { return ResourceState.CurrentValue; }
	float GetMinimum() const { return ResourceState.Minimum; }
	float GetMaximum() const { return ResourceState.Maximum; }
	bool GetInitialized() const { return bInitialized; }
	bool GetDeactivated() const { return bDeactivated; }
};