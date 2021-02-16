// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

public:

	static const FGameplayTag GenericStatTag;
	static const FGameplayTag GenericStatModTag;
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool GetStatValid(FGameplayTag const& StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(FGameplayTag const& StatTag) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void AddStatModifier(FGameplayTag const& StatTag, FStatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void RemoveStatModifier(FGameplayTag const& StatTag, FStatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
    void UpdateModifiedStat(FGameplayTag const& StatTag);
	
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback);
	
	UFUNCTION(meta = (GameplayTagFilter = "Stat"))
	void NotifyOfReplicatedStat(FGameplayTag const& StatTag, float const NewValue);

private:

	void RecalculateStat(FGameplayTag const& StatTag);

	//Can't be UFUNCTION because struct pointer returns aren't allowed by reflection.
	FStatInfo* GetStatInfoPtr(FGameplayTag const& StatTag);
	//Can't be UFUNCTION because struct pointer returns aren't allowed by reflection.
	FStatInfo const* GetStatInfoConstPtr(FGameplayTag const& StatTag) const;

	UFUNCTION()
	bool CheckBuffStatMods(FBuffApplyEvent const& BuffEvent);

	UPROPERTY(EditAnywhere, Category = "Stat", meta = (Categories = "Stat"))
	TMap<FGameplayTag, FStatInfo> StatInfo;
	UPROPERTY(Replicated)
	FReplicatedStatArray ReplicatedStats;
	UPROPERTY(Replicated)
	FReplicatedStatArray OwnerOnlyStats;

	FGameplayTagContainer UnmodifiableStats;
};
