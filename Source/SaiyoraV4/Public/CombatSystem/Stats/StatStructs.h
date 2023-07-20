#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatStructs.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatCallback, const FGameplayTag, StatTag, const float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatNotification, const FGameplayTag&, StatTag, const float, NewValue);

USTRUCT()
struct FStatInitInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta = (Categories = "Stat"))
    FGameplayTag StatTag;
    UPROPERTY(EditAnywhere)
    float DefaultValue = 1.0f;
    UPROPERTY(EditAnywhere)
    bool bModifiable = true;
    UPROPERTY(EditAnywhere)
    bool bUseCustomMin = false;
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
    float CustomMin = 0.0f;
    UPROPERTY(EditAnywhere)
    bool bUseCustomMax = false;
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
    float CustomMax = 0.0f;
};

USTRUCT()
struct FCombatStat : public FFastArraySerializerItem
{
    GENERATED_BODY()

    void PostReplicatedAdd(const struct FCombatStatArray& InArraySerializer);
    void PostReplicatedChange(const struct FCombatStatArray& InArraySerializer);

    UPROPERTY(EditAnywhere, NotReplicated, meta = (Categories = "Stat"))
    FGameplayTag StatTag;
    UPROPERTY(EditAnywhere)
    FModifiableFloat StatValue;
    
    FStatNotification OnStatChanged;
    bool bInitialized = false;
    
    //TODO: Add global stat replication rules somewhere to check whether to mark a stat dirty when it changes, probably in CombatStructs with a set of Replicated Tags.
    //Currently all stats replicate to everyone.

    FCombatStat() {}
    FCombatStat(const FStatInitInfo* InitInfo)
    {
        StatTag = InitInfo->StatTag;
        UE_LOG(LogTemp, Warning, TEXT("Initializing %s stat"), *StatTag.ToString());
        StatValue = FModifiableFloat(InitInfo->DefaultValue, InitInfo->bModifiable, InitInfo->bUseCustomMin, InitInfo->CustomMin, InitInfo->bUseCustomMax, InitInfo->CustomMax);
    }
};

USTRUCT()
struct FCombatStatArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FCombatStat> Items;
    UPROPERTY(NotReplicated)
    class UStatHandler* Handler = nullptr;
    TMultiMap<FGameplayTag, FStatCallback> PendingSubscriptions;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FCombatStat, FCombatStatArray>( Items, DeltaParms, *this );
    }
};

template<>
struct TStructOpsTypeTraits<FCombatStatArray> : public TStructOpsTypeTraitsBase2<FCombatStatArray>
{
    enum 
    {
        WithNetDeltaSerializer = true,
    };
};