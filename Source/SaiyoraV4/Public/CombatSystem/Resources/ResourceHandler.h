#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceStructs.h"
#include "Resource.h"
#include "AbilityStructs.h"
#include "ResourceHandler.generated.h"

class UCombatAbility;
class UStatHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UResourceHandler : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UResourceHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

//Resource Management

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	UResource* FindActiveResource(TSubclassOf<UResource> const ResourceClass) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	void GetActiveResources(TArray<UResource*>& OutResources) const { OutResources = ActiveResources; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void AddNewResource(TSubclassOf<UResource> const ResourceClass, FResourceInitInfo const& InitInfo);
	void NotifyOfReplicatedResource(UResource* Resource);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void RemoveResource(TSubclassOf<UResource> const ResourceClass);
	void NotifyOfRemovedReplicatedResource(UResource* Resource);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceRemoved(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceRemoved(FResourceInstanceCallback const& Callback);

private:

	UPROPERTY(EditAnywhere, Category = "Resource")
	TMap<TSubclassOf<UResource>, FResourceInitInfo> DefaultResources;
	UPROPERTY()
	TArray<UResource*> ActiveResources;
	UPROPERTY()
	TArray<UResource*> RecentlyRemovedResources;
	UFUNCTION()
	void FinishRemoveResource(UResource* Resource);
	FResourceInstanceNotification OnResourceAdded;
	FResourceInstanceNotification OnResourceRemoved;

//Ability Costs

public:
	
	void CommitAbilityCosts(UCombatAbility* Ability, int32 const PredictionID = 0);
	void UpdatePredictedCostsFromServer(struct FServerAbilityResult const& ServerResult);

private:

	TMultiMap<int32, FAbilityCost> CostPredictions;
};