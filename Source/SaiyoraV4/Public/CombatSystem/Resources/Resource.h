#pragma once
#include "CoreMinimal.h"
#include "ResourceStructs.h"
#include "GameplayTagContainer.h"
#include "StatStructs.h"
#include "Styling/SlateBrush.h"
#include "Resource.generated.h"

class UCombatAbility;
class UResourceHandler;
class UStatHandler;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UResource : public UObject
{
	GENERATED_BODY()

//Initialization/Deactivation

public:

	virtual UWorld* GetWorld() const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostNetReceive() override;

	void InitializeResource(UResourceHandler* NewHandler, FResourceInitInfo const& InitInfo);
	bool IsInitialized() const { return bInitialized; }
	void DeactivateResource();
	bool IsDeactivated() const { return bDeactivated; }

protected:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	UResourceHandler* GetHandler() const { return Handler; }
	UFUNCTION(BlueprintNativeEvent)
	void PreInitializeResource();
	virtual void PreInitializeResource_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void PreDeactivateResource();
	virtual void PreDeactivateResource_Implementation() {}

private:

	UPROPERTY(Replicated)
	UResourceHandler* Handler = nullptr;
	bool bInitialized = false;
	UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
	bool bDeactivated = false;
	UFUNCTION()
	void OnRep_Deactivated();
	UPROPERTY()
	UStatHandler* StatHandlerRef = nullptr;

//Display Info

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	FSlateBrush GetDisplayFillBrush() const { return BarFillBrush; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	FSlateBrush GetDisplayBackgroundBrush() const { return BarBackgroundBrush; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	FSlateBrush BarFillBrush;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	FSlateBrush BarBackgroundBrush;

//Init Info

private:

	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMinimum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MinimumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMaximum = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MaximumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultValue = 0.0f;

//State

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetCurrentValue() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMinimum() const { return ResourceState.Minimum; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMaximum() const { return ResourceState.Maximum; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void ModifyResource(UObject* Source, float const Amount, bool const bIgnoreModifiers);
	void AddResourceDeltaModifier(UBuff* Source, FResourceDeltaModifier const& Modifier);
	void RemoveResourceDeltaModifier(UBuff* Source);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceChanged(FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback);

protected:

	UFUNCTION(BlueprintNativeEvent)
	void PostResourceUpdated(UObject* Source, FResourceState const& PreviousState);
	virtual void PostResourceUpdated_Implementation(UObject* Source, FResourceState const& PreviousState) {}

private:

	UPROPERTY(ReplicatedUsing = OnRep_ResourceState)
	FResourceState ResourceState;
	UFUNCTION()
	void OnRep_ResourceState(FResourceState const& PreviousState);
	FStatCallback MinStatBind;
	UFUNCTION()
	void UpdateMinimumFromStatBind(FGameplayTag const& StatTag, float const NewValue);
	FStatCallback MaxStatBind;
	UFUNCTION()
	void UpdateMaximumFromStatBind(FGameplayTag const& StatTag, float const NewValue);
	void SetResourceValue(float const NewValue, UObject* Source, int32 const PredictionID = 0);
	TMap<UBuff*, FResourceDeltaModifier> ResourceDeltaMods;
	FResourceValueNotification OnResourceChanged;
	
//Ability Costs
	
public:

	void CommitAbilityCost(UCombatAbility* Ability, float const Cost, int32 const PredictionID = 0);
	void UpdateCostPredictionFromServer(int32 const PredictionID, float const ServerCost);
	
private:

	float PredictedResourceValue = 0.0f;
	TMap<int32, float> ResourcePredictions;
	void RecalculatePredictedResource(UObject* ChangeSource);
	void PurgeOldPredictions();
};