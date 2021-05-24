// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraStructs.h"
#include "SaiyoraEnums.h"
#include "GameplayTagContainer.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatCallback, FGameplayTag const&, StatTag, float const, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatNotification, FGameplayTag const&, StatTag, float const, NewValue);

USTRUCT(BlueprintType)
struct FStatModifier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, meta = (GameplayTagFilter = "Stat"))
    FGameplayTag StatTag;
    UPROPERTY(BlueprintReadWrite)
    EModifierType ModType = EModifierType::Invalid;
    UPROPERTY(BlueprintReadWrite)
    float ModValue = 0.0f;
    UPROPERTY(BlueprintReadWrite)
    bool bStackable = false;

    FORCEINLINE bool operator==(const FStatModifier& Other) const { return Other.StatTag.MatchesTagExact(StatTag) && Other.ModType == ModType && Other.ModValue == ModValue && Other.bStackable == bStackable; }
};

USTRUCT()
struct FStatInfo
{
    GENERATED_BODY()
  
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float DefaultValue = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bModifiable = false;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bCappedLow = false;
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float MinClamp = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bCappedHigh = false;
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float MaxClamp = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    EReplicationNeeds ReplicationNeeds = EReplicationNeeds::NotReplicated;

    UPROPERTY()
    bool bInitialized = false;
    UPROPERTY()
    float CurrentValue = 0.0f;
    UPROPERTY()
    FStatNotification OnStatChanged;
    UPROPERTY()
    TArray<FCombatModifier> StatModifiers;
};

USTRUCT()
struct FReplicatedStat : public FFastArraySerializerItem
{
    GENERATED_BODY()

    void PostReplicatedChange(struct FReplicatedStatArray const& InArraySerializer);
    void PostReplicatedAdd(struct FReplicatedStatArray const& InArraySerializer);

    UPROPERTY(meta = (Categories = "Stat"))
    FGameplayTag StatTag;
    UPROPERTY()
    float Value = 0.0f;
};

USTRUCT()
struct FReplicatedStatArray : public FFastArraySerializer
{
    GENERATED_BODY()

    void UpdateStatValue(FGameplayTag const& StatTag, float const NewValue);

    UPROPERTY()
    TArray<FReplicatedStat> Items;
    UPROPERTY()
    class UStatHandler* StatHandler;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedStat, FReplicatedStatArray>( Items, DeltaParms, *this );
    }
};

template<>
struct TStructOpsTypeTraits<FReplicatedStatArray> : public TStructOpsTypeTraitsBase2<FReplicatedStatArray>
{
    enum 
    {
        WithNetDeltaSerializer = true,
   };
};