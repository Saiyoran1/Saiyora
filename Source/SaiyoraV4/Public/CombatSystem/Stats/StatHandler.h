// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

class UBuffHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

public:

	static const FGameplayTag GenericStatTag;
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool GetStatValid(FGameplayTag const& StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(FGameplayTag const& StatTag) const;

	void AddStatModifier(FStatModifier const& Modifier);
	void AddStatModifiers(TArray<FStatModifier> const& Modifiers);
	void RemoveStatModifier(FStatModifier const& Modifier);
	void RemoveStatModifiers(UBuff* Source, TSet<FGameplayTag> const& AffectedStats);
    void UpdateStackingModifiers(UBuff* Source, TSet<FGameplayTag> const& AffectedStats);
	
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback);
	
	UFUNCTION(meta = (GameplayTagFilter = "Stat"))
	void NotifyOfReplicatedStat(FGameplayTag const& StatTag, float const NewValue);

private:

	void RecalculateStat(FGameplayTag const& StatTag);

	FStatInfo* GetStatInfoPtr(FGameplayTag const& StatTag);
	FStatInfo const* GetStatInfoConstPtr(FGameplayTag const& StatTag) const;

	UFUNCTION()
	bool CheckBuffStatMods(FBuffApplyEvent const& BuffEvent) const;

	UPROPERTY(EditAnywhere, Category = "Stat", meta = (Categories = "Stat"))
	TMap<FGameplayTag, FStatInfo> StatInfo;
	UPROPERTY(Replicated)
	FReplicatedStatArray ReplicatedStats;
	UPROPERTY(Replicated)
	FReplicatedStatArray OwnerOnlyStats;

	FGameplayTagContainer UnmodifiableStats;

	UPROPERTY()
	UBuffHandler* BuffHandler;
};
