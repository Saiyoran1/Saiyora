#pragma once
#include "CoreMinimal.h"
#include "CrowdControlEnums.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "CrowdControlStructs.generated.h"

class UCrowdControlHandler;
class UBuff;

//Struct representing the state of one type of crowd control for an actor
USTRUCT(BlueprintType)
struct FCrowdControlStatus : public FFastArraySerializerItem
{
    GENERATED_BODY();

	//What type of crowd control this status is tracking.
	UPROPERTY(BlueprintReadOnly)
	ECrowdControl CrowdControlType = ECrowdControl::Stun;

	//Whether the crowd control is currently active.
	UPROPERTY(BlueprintReadOnly)
	bool bActive = false;
	//Buffs that are applying this kind of crowd control.
	UPROPERTY(NotReplicated)
	TSet<UBuff*> Sources;
	//The class of the buff applying this crowd control that has the longest remaining duration.
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> DominantBuffClass;
	//Pointer to the buff applying this crowd control that has the longest remaining duration.
    UPROPERTY(NotReplicated)
    UBuff* DominantBuffInstance = nullptr;
	//The time this crowd control is expected to end, based on the dominant buff.
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;

	//Whether we are immune to this crowd control type by default.
	UPROPERTY(NotReplicated)
	bool bDefaultImmuned = false;
	//Buffs currently providing immunity to this crowd control.
	UPROPERTY(NotReplicated)
	TSet<UBuff*> Immunities;
	
    bool AddNewBuff(UBuff* Source);
    bool RemoveBuff(UBuff* Source);
    bool RefreshBuff(UBuff* Source);

	void PostReplicatedAdd(const struct FCrowdControlStatusArray& InArraySerializer);
	void PostReplicatedChange(const struct FCrowdControlStatusArray& InArraySerializer);

private:

	//Called when a new buff is added or a buff is refreshed that might now have a longer duration than the previous dominant buff
    void SetNewDominantBuff(UBuff* NewDominant);
	//Helper function for finding the longest duration buff applying this crowd control
    UBuff* GetLongestBuff();
};

USTRUCT()
struct FCrowdControlStatusArray : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FCrowdControlStatus> Items;
	
	void Init(UCrowdControlHandler* InHandler);
	UCrowdControlHandler* GetHandler() const { return OwningHandler; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FCrowdControlStatus, FCrowdControlStatusArray>(Items, DeltaParms, *this);
	}

private:
	
	UPROPERTY(NotReplicated)
	UCrowdControlHandler* OwningHandler = nullptr;
};

template<>
struct TStructOpsTypeTraits<FCrowdControlStatusArray> : public TStructOpsTypeTraitsBase2<FCrowdControlStatusArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};

//Multicast delegate for notifying of a change in the state of a crowd control
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCrowdControlNotification, const FCrowdControlStatus&, NewStatus);