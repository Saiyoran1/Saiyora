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

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UWorld* GetWorld() const override;
	virtual void PostNetReceive() override;

	void InitializeResource(UResourceHandler* NewHandler, UStatHandler* StatHandler, FResourceInitInfo const& InitInfo);
	bool GetInitialized() const { return bInitialized; }
	void DeactivateResource();
	bool GetDeactivated() const { return bDeactivated; }

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

public:

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

	UPROPERTY(ReplicatedUsing = OnRep_ResourceState)
	FResourceState ResourceState;
	FResourceState PredictedResourceState;
	TMap<int32, float> ResourcePredictions;
	void RecalculatePredictedResource(UObject* ChangeSource);
	void PurgeOldPredictions();
	UFUNCTION()
	void OnRep_ResourceState(FResourceState const& PreviousState);

	void SetNewMinimum(float const NewValue);
	void SetNewMaximum(float const NewValue);
	void SetResourceValue(float const NewValue, UObject* Source, int32 const PredictionID = 0);

	FStatCallback MinStatBind;
	UFUNCTION()
	void UpdateMinimumFromStatBind(FGameplayTag const& StatTag, float const NewValue);
	FStatCallback MaxStatBind;
	UFUNCTION()
	void UpdateMaximumFromStatBind(FGameplayTag const& StatTag, float const NewValue);

	TMap<int32, FResourceGainModifier> ResourceGainMods;

	FResourceValueNotification OnResourceChanged;

protected:

	
	UFUNCTION(BlueprintImplementableEvent)
	void OnResourceUpdated(UObject* Source, FResourceState const& PreviousState, FResourceState const& NewState);
	
public:

	void CommitAbilityCost(UCombatAbility* Ability, float const Cost, int32 const PredictionID = 0);
	void PredictAbilityCost(UCombatAbility* Ability, int32 const PredictionID, float const Cost);
	void RollbackFailedCost(int32 const PredictionID);
	void UpdateCostPredictionFromServer(int32 const PredictionID, float const ServerCost);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void ModifyResource(UObject* Source, float const Amount, bool const bIgnoreModifiers);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	int32 AddResourceGainModifier(FResourceGainModifier const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void RemoveResourceGainModifier(int32 const ModifierID);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceChanged(FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetCurrentValue() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMinimum() const { return ResourceState.Minimum; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	float GetMaximum() const { return ResourceState.Maximum; }
	
	
};