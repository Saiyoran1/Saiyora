// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "Components/ActorComponent.h"
#include "ResourceStructs.h"
#include "Resource.h"
#include "ResourceHandler.generated.h"

struct FFinalAbilityCost;
class UCombatAbility;
class UStatHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UResourceHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UResourceHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void AddNewResource(TSubclassOf<UResource> const ResourceClass, FResourceInitInfo const& InitInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void RemoveResource(TSubclassOf<UResource> const ResourceClass);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	UResource* FindActiveResource(TSubclassOf<UResource> const ResourceClass) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	TArray<UResource*> GetActiveResources() const { return ActiveResources; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	bool CheckAbilityCostsMet(TMap<TSubclassOf<UResource>, float> const& Costs) const;
	void CommitAbilityCosts(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float> const& Costs, int32 const PredictionID = 0);
	void PredictAbilityCosts(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float> const& Costs, int32 const PredictionID);
	void UpdatePredictedCostsFromServer(FServerAbilityResult const& ServerResult, TArray<TSubclassOf<UResource>> const& MispredictedCosts);
	void RollbackFailedCosts(TArray<TSubclassOf<UResource>> const& CostClasses, int32 const PredictionID);

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void UnsubscribeFromResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceRemoved(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceRemoved(FResourceInstanceCallback const& Callback);

	//Client-side updating, creation, and removal of resource objects.
	void NotifyOfReplicatedResource(UResource* Resource);
	void NotifyOfRemovedReplicatedResource(UResource* Resource);

private:

	UPROPERTY(EditAnywhere, Category = "Resource")
	TMap<TSubclassOf<UResource>, FResourceInitInfo> DefaultResources;
	UPROPERTY()
	TArray<UResource*> ActiveResources;
	UPROPERTY()
	TArray<UResource*> RecentlyRemovedResources;

	FResourceInstanceNotification OnResourceAdded;
	FResourceInstanceNotification OnResourceRemoved;

	UFUNCTION()
    void FinishRemoveResource(UResource* Resource);
	bool CheckResourceAlreadyExists(TSubclassOf<UResource> const ResourceClass) const;

	UPROPERTY()
	UStatHandler* StatHandler;
};