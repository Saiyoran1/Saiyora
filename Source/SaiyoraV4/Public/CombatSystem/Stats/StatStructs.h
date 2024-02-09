#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatStructs.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatCallback, const FGameplayTag, StatTag, const float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatNotification, const FGameplayTag&, StatTag, const float, NewValue);

//Data table row struct for initializing stats.
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
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseCustomMin", ClampMin = "0"))
    float CustomMin = 0.0f;
    UPROPERTY(EditAnywhere)
    bool bUseCustomMax = false;
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseCustomMax", ClampMin = "0"))
    float CustomMax = 0.0f;
};

USTRUCT()
struct FCombatStat : public FModifiableFloat
{
    GENERATED_BODY()

    void PostReplicatedAdd(const struct FCombatStatArray& InArraySerializer);
    void PostReplicatedChange(const struct FCombatStatArray& InArraySerializer);

    UPROPERTY(EditAnywhere, meta = (Categories = "Stat"))
    FGameplayTag StatTag;
    
    FStatNotification OnStatChanged;
    bool bInitialized = false;
    
    //TODO: Add global stat replication rules somewhere to check whether to mark a stat dirty when it changes, probably in CombatStructs with a set of Replicated Tags.
    //Currently all stats replicate to everyone.

    FCombatStat() {}
    FCombatStat(const FStatInitInfo* InitInfo) :
        Super(InitInfo->DefaultValue, InitInfo->bModifiable, InitInfo->bUseCustomMin, InitInfo->CustomMin, InitInfo->bUseCustomMax, InitInfo->CustomMax)
    {
        StatTag = InitInfo->StatTag;
    }
};

USTRUCT()
struct FCombatStatArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FCombatStat> Items;
    //If something wants to subscribe to a stat before it has been replicated, it is put into a pending subscription list.
    //Delegates in this list are bound and then executed when the relevant stat is replicated to the client.
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