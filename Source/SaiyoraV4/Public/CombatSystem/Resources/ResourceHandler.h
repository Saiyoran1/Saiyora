// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "Components/ActorComponent.h"
#include "ResourceStructs.h"
#include "GameplayTagContainer.h"
#include "ResourceHandler.generated.h"

struct FFinalAbilityCost;
class UResource;
class UCombatAbility;
class UStatHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UResourceHandler : public UActorComponent
{
	GENERATED_BODY()

public:

	static const FGameplayTag GenericResourceTag;
	
	UResourceHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	UResource* FindActiveResource(FGameplayTag const& ResourceTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
	FGameplayTagContainer GetActiveResources() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	float GetResourceValue(FGameplayTag const& ResourceTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	float GetResourceMinimum(FGameplayTag const& ResourceTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	float GetResourceMaximum(FGameplayTag const& ResourceTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	bool CheckAbilityCostsMet(UCombatAbility* Ability, TArray<FAbilityCost>& OutCosts) const;
	void CommitAbilityCosts(UCombatAbility* Ability, int32 const CastID, TArray<FAbilityCost> const& Costs);

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void UnsubscribeFromResourceAdded(FResourceInstanceCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SubscribeToResourceRemoved(FResourceTagCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UnsubscribeFromResourceRemoved(FResourceTagCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	void SubscribeToResourceChanged(FGameplayTag const& ResourceTag, FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	void UnsubscribeFromResourceChanged(FGameplayTag const& ResourceTag, FResourceValueCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
	void SubscribeToResourceModsChanged(FGameplayTag const& ResourceTag, FResourceTagCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resource", meta = (GameplayTagFilter = "Resource"))
    void UnsubscribeFromResourceModsChanged(FGameplayTag const& ResourceTag, FResourceTagCallback const& Callback);

	//Client-side updating, creation, and removal of resource objects.
	void NotifyOfReplicatedResource(UResource* Resource);
	void NotifyOfRemovedReplicatedResource(FGameplayTag const& ResourceTag);

private:

	UPROPERTY(EditAnywhere, Category = "Resource")
	TArray<FResourceInitInfo> DefaultResources;
	UPROPERTY()
	TMap<FGameplayTag, UResource*> ActiveResources;
	UPROPERTY()
	TArray<UResource*> RecentlyRemovedResources;

	FResourceInstanceNotification OnResourceAdded;
	FResourceTagNotification OnResourceRemoved;

	void AddNewResource(FResourceInitInfo const& InitInfo);
	void RemoveResource(FGameplayTag const& ResourceTag);
	UFUNCTION()
    void FinishRemoveResource(UResource* Resource);
	bool CheckResourceAlreadyExists(FGameplayTag const& ResourceTag) const;

	UPROPERTY()
	UStatHandler* StatHandler;
};