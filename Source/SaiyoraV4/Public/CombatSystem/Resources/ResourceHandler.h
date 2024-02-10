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

#pragma region Init

public:
	
	UResourceHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion 
#pragma region Resource Management

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

	//Resources this actor will start with.
	UPROPERTY(EditAnywhere, Category = "Resource")
	TMap<TSubclassOf<UResource>, FResourceInitInfo> DefaultResources;
	UPROPERTY()
	TArray<UResource*> ActiveResources;
	//These resources have been removed but are kept around on the server to allow replication of their removal to work correctly on clients.
	UPROPERTY()
	TArray<UResource*> RecentlyRemovedResources;
	//Called after a delay when removing a resource on the server, to actually get rid of the reference.
	UFUNCTION()
	void FinishRemoveResource(UResource* Resource);

#pragma endregion 
#pragma region Ability Costs

public:

	//This commits the ability costs either on the server or on a predicting client.
	void CommitAbilityCosts(UCombatAbility* Ability, const int32 PredictionID = 0);
	//Called after a client receives information from the server about their ability prediction.
	//This confirms or corrects predicted costs using the verified server information.
	void UpdatePredictedCostsFromServer(const FServerAbilityResult& ServerResult);

private:

	TMultiMap<int32, FSimpleAbilityCost> CostPredictions;

#pragma endregion 
};