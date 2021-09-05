// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "CombatStat.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
public:
	static FGameplayTag GenericStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false); }
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool GetStatValid(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	int32 AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void RemoveStatModifier(FGameplayTag const StatTag, int32 const ModifierID);	
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
private:
	UPROPERTY(EditAnywhere, Category = "Stat")
	UDataTable* InitialStats;
	UPROPERTY()
	TMap<FGameplayTag, UCombatStat*> Stats;
	FGameplayTagContainer UnmodifiableStats;
	UPROPERTY()
	class UBuffHandler* BuffHandler;
	UFUNCTION()
	bool CheckBuffStatMods(FBuffApplyEvent const& BuffEvent);
};
