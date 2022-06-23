#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceStructs.h"
#include "AbilityStructs.h"
#include "ResourceHandler.generated.h"

class UCombatAbility;
class UStatHandler;
struct FServerAbilityResult;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UResourceHandler : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UResourceHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

//Resource Management

public:

	UFUNCTION(BlueprintPure, Category = "Resource")
	UResource* FindActiveResource(const TSubclassOf<UResource> ResourceClass) const;
	UFUNCTION(BlueprintPure, Category = "Resource")
	void GetActiveResources(TArray<UResource*>& OutResources) const { OutResources = ActiveResources; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void AddNewResource(const TSubclassOf<UResource> ResourceClass, const FResourceInitInfo& InitInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources")
	void RemoveResource(const TSubclassOf<UResource> ResourceClass);
	UPROPERTY(BlueprintAssignable)
	FResourceInstanceNotification OnResourceAdded;
	UPROPERTY(BlueprintAssignable)
	FResourceInstanceNotification OnResourceRemoved;

	void NotifyOfReplicatedResource(UResource* Resource);
	void NotifyOfRemovedReplicatedResource(UResource* Resource);
	
private:

	UPROPERTY(EditAnywhere, Category = "Resource")
	TMap<TSubclassOf<UResource>, FResourceInitInfo> DefaultResources;
	UPROPERTY()
	TArray<UResource*> ActiveResources;
	UPROPERTY()
	TArray<UResource*> RecentlyRemovedResources;
	UFUNCTION()
	void FinishRemoveResource(UResource* Resource);

//Ability Costs

public:
	
	void CommitAbilityCosts(UCombatAbility* Ability, const int32 PredictionID = 0);
	void UpdatePredictedCostsFromServer(const FServerAbilityResult& ServerResult);

private:

	TMultiMap<int32, FAbilityCost> CostPredictions;
};