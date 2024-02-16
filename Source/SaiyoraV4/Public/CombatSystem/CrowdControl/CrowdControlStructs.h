#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CrowdControlStructs.generated.h"

class UBuff;

//Struct representing the state of one type of crowd control for an actor
USTRUCT(BlueprintType)
struct FCrowdControlStatus
{
    GENERATED_BODY();

	//Tag to indicate the crowd control type
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CrowdControlType;
	//Buffs that are applying this kind of crowd control
    UPROPERTY(NotReplicated)
    TArray<UBuff*> Sources;
	//Whether the crowd control is currently active
    UPROPERTY(BlueprintReadOnly)
    bool bActive = false;
	//The class of the buff applying this crowd control that has the longest remaining duration
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> DominantBuffClass;
	//Pointer to the buff applying this crowd control that has the longest remaining duration
    UPROPERTY(NotReplicated)
    UBuff* DominantBuffInstance = nullptr;
	//The time this crowd control is expected to end, based on the dominant buff
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;
    bool AddNewBuff(UBuff* Source);
    bool RemoveBuff(UBuff* Source);
    bool RefreshBuff(UBuff* Source);

private:

	//Called when a new buff is added or a buff is refreshed that might now have a longer duration than the previous dominant buff
    void SetNewDominantBuff(UBuff* NewDominant);
	//Helper function for finding the longest duration buff applying this crowd control
    UBuff* GetLongestBuff();
};

//Multicast delegate for notifying of a change in the state of a crowd control
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCrowdControlNotification, const FCrowdControlStatus&, PreviousStatus, const FCrowdControlStatus&, NewStatus);