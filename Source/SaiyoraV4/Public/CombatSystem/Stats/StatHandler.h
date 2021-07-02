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

	//static const FGameplayTag GenericStatTag;
	static FGameplayTag GenericStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false); }
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool GetStatValid(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(FGameplayTag const StatTag) const;

	//Individual Stat Mod functions.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	int32 AddStatModifier(FGameplayTag const StatTag, EModifierType const ModType, float const ModValue);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void RemoveStatModifier(FGameplayTag const StatTag, int32 const ModifierID);

	//Mass Stat Mod functions from buffs.
	void AddStatModifiers(TMultiMap<FGameplayTag, FCombatModifier> const& Modifiers);
	void RemoveStatModifiers(TSet<FGameplayTag> const& AffectedStats, UBuff* Source);
    void UpdateStackingModifiers(TSet<FGameplayTag> const& AffectedStats);
	
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
	
	UFUNCTION()
	void NotifyOfReplicatedStat(FGameplayTag const& StatTag, float const NewValue);

private:

	void RecalculateStat(FGameplayTag const& StatTag);

	FStatInfo* GetStatInfoPtr(FGameplayTag const& StatTag);
	FStatInfo const* GetStatInfoConstPtr(FGameplayTag const& StatTag) const;

	UFUNCTION()
	bool CheckBuffStatMods(FBuffApplyEvent const& BuffEvent);
	
	UPROPERTY(EditAnywhere, Category = "Stat")
	UDataTable* InitialStats;
	TMap<FGameplayTag, FStatInfo> StatInfo;
	UPROPERTY(Replicated)
	FReplicatedStatArray ReplicatedStats;
	UPROPERTY(Replicated)
	FReplicatedStatArray OwnerOnlyStats;

	FGameplayTagContainer UnmodifiableStats;

	UPROPERTY()
	UBuffHandler* BuffHandler;
};
