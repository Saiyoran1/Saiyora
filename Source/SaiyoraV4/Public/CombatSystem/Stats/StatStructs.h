#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatStructs.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatCallback, const FGameplayTag, StatTag, const float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatNotification, const FGameplayTag&, StatTag, const float, NewValue);

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
    UPROPERTY(EditAnywhere, NotReplicated)
    bool bUseCustomMin = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (ClampMin = "0"))
    float CustomMin = 0.0f;
    UPROPERTY(EditAnywhere, NotReplicated)
    bool bUseCustomMax = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (ClampMin = "0"))
    float CustomMax = 0.0f;
    
    FStatNotification OnStatChanged;
    bool bInitialized = false;
    //TODO: Add global stat replication rules somewhere to check whether to mark a stat dirty when it changes, probably in CombatStructs with a set of Replicated Tags.
    //Currently all stats replicate to everyone.
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