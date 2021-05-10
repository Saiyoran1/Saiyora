// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CrowdControlStructs.generated.h"

class UBuff;
class UCrowdControl;
class UCrowdControlHandler;

USTRUCT(BlueprintType)
struct FCrowdControlStatus
{
    GENERATED_BODY();

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UCrowdControl> CrowdControlClass;
    UPROPERTY(BlueprintReadOnly)
    bool bActive = false;
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> DominantBuff;
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCrowdControlCallback, FCrowdControlStatus const&, PreviousStatus, FCrowdControlStatus const&, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCrowdControlNotification, FCrowdControlStatus const&, PreviousStatus, FCrowdControlStatus const&, NewStatus);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FCrowdControlRestriction, UBuff*, Source, TSubclassOf<UCrowdControl>, CrowdControlType);

USTRUCT()
struct FReplicatedCrowdControl : public FFastArraySerializerItem
{
    GENERATED_BODY();

    UPROPERTY()
    TSubclassOf<UCrowdControl> CrowdControlClass;
    UPROPERTY()
    bool bActive = false;
    UPROPERTY()
    TSubclassOf<UBuff> DominantBuff;
    UPROPERTY()
    float EndTime = 0.0f;

    void PostReplicatedAdd(const struct FReplicatedCrowdControlArray& InArraySerializer);
    void PostReplicatedChange(const struct FReplicatedCrowdControlArray& InArraySerializer);
    void PreReplicatedRemove(const struct FReplicatedCrowdControlArray& InArraySerializer);
};

USTRUCT()
struct FReplicatedCrowdControlArray : public FFastArraySerializer
{
    GENERATED_BODY();

    UPROPERTY()
    TArray<FReplicatedCrowdControl> Items;
    UPROPERTY()
    UCrowdControlHandler* Handler;

    void UpdateArray(FCrowdControlStatus const& NewStatus);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedCrowdControl, FReplicatedCrowdControlArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FReplicatedCrowdControlArray> : public TStructOpsTypeTraitsBase2<FReplicatedCrowdControlArray>
{
    enum
    {
        WithNetDeltaSerializer = true
    };
};
