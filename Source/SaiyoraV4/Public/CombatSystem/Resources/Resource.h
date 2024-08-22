#pragma once
#include "CoreMinimal.h"
#include "ResourceStructs.h"
#include "GameplayTagContainer.h"
#include "StatStructs.h"
#include "Styling/SlateBrush.h"
#include "Resource.generated.h"

class UResourceBar;
class UCombatAbility;
class UResourceHandler;
class UStatHandler;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UResource : public UObject
{
	GENERATED_BODY()

#pragma region Initialization and Deactivation

public:

	virtual UWorld* GetWorld() const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostNetReceive() override;

	void InitializeResource(UResourceHandler* NewHandler, const FResourceInitInfo& InitInfo);
	bool IsInitialized() const { return bInitialized; }
	void DeactivateResource();
	bool IsDeactivated() const { return bDeactivated; }

protected:

	UFUNCTION(BlueprintPure, Category = "Resource")
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
	
#pragma endregion 
#pragma region Display

public:

	TSubclassOf<UResourceBar> GetHUDResourceWidget() const { return HUDResourceWidget; }
	TSubclassOf<UResourceBar> GetNameplateResourceWidget() const { return NameplateResourceWidget; }
	TSubclassOf<UResourceBar> GetPartyFrameResourceWidget() const { return PartyFrameResourceWidget; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	TSubclassOf<UResourceBar> HUDResourceWidget;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	TSubclassOf<UResourceBar> NameplateResourceWidget;
	UPROPERTY(EditDefaultsOnly, Category = "Resource")
	TSubclassOf<UResourceBar> PartyFrameResourceWidget;

#pragma endregion 
#pragma region State

public:

	//Get the current value of the resource, or the client's predicted value if not on the server.
	UFUNCTION(BlueprintPure, Category = "Resource")
	float GetCurrentValue() const;
	UFUNCTION(BlueprintPure, Category = "Resource")
	float GetMinimum() const { return ResourceState.Minimum; }
	UFUNCTION(BlueprintPure, Category = "Resource")
	float GetMaximum() const { return ResourceState.Maximum; }
	//Used to adjust the resource value for everything that isn't an ability cost.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource")
	void ModifyResource(UObject* Source, const float Amount, const bool bIgnoreModifiers);
	UPROPERTY(BlueprintAssignable)
	FResourceValueNotification OnResourceChanged;

	//Add a modifier to non-ability cost resource gains and losses.
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void AddResourceDeltaModifier(const FResourceDeltaModifier& Modifier) { ResourceDeltaMods.Add(Modifier); }
	//Remove a modifier from non-ability cost resource gains and losses.
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void RemoveResourceDeltaModifier(const FResourceDeltaModifier& Modifier) { ResourceDeltaMods.Remove(Modifier); }

protected:

	//Function for inherited resources to trigger their own behavior on values changing.
	UFUNCTION(BlueprintNativeEvent)
	void PostResourceUpdated(UObject* Source, const FResourceState& PreviousState);
	virtual void PostResourceUpdated_Implementation(UObject* Source, const FResourceState& PreviousState) {}

private:

	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMinimum = 0.0f;
	//Setting this allows the resource's minimum value to be bound to a stat's value, if the actor has that stat.
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MinimumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultMaximum = 0.0f;
	//Setting this allows the resource's maximum value to be bound to a stat's value, if the actor has that stat.
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (GameplayTagFilter = "Stat"))
	FGameplayTag MaximumBindStat;
	UPROPERTY(EditDefaultsOnly, Category = "Resource", meta = (ClampMin = "0"))
	float DefaultValue = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_ResourceState)
	FResourceState ResourceState;
	UFUNCTION()
	void OnRep_ResourceState(const FResourceState& PreviousState);
	FStatCallback MinStatBind;
	UFUNCTION()
	void UpdateMinimumFromStatBind(const FGameplayTag StatTag, const float NewValue);
	FStatCallback MaxStatBind;
	UFUNCTION()
	void UpdateMaximumFromStatBind(const FGameplayTag StatTag, const float NewValue);
	void SetResourceValue(const float NewValue, UObject* Source, const int32 PredictionID = 0);
	
	TConditionalModifierList<FResourceDeltaModifier> ResourceDeltaMods;

#pragma endregion 
#pragma region Ability Costs
	
public:

	//Called on both the server and predicting clients to commit the cost of an ability.
	void CommitAbilityCost(UCombatAbility* Ability, const float Cost, const int32 PredictionID = 0);
	//Called when clients receive the server ack of their ability prediction with verified costs.
	//Confirms or adjusts predictions the client made originally when predicting the ability use.
	void UpdateCostPredictionFromServer(const int32 PredictionID, const float ServerCost);
	
private:

	//The local value of the resource after any client cost predictions are applied.
	float PredictedResourceValue = 0.0f;
	TMap<int32, float> ResourcePredictions;
	void RecalculatePredictedResource(UObject* ChangeSource);
	//When resource is state is replicated, it contains the latest prediction ID that updated that resource.
	//This allows us to remove old predictions that the server has already taken into account.
	void PurgeOldPredictions();

#pragma endregion 
};