#pragma once
#include "CoreMinimal.h"
#include "CrowdControlEnums.h"
#include "CrowdControlStructs.generated.h"

class UBuff;

USTRUCT(BlueprintType)
struct FCrowdControlStatus
{
    GENERATED_BODY();

    UPROPERTY(NotReplicated)
    TArray<UBuff*> Sources;
    UPROPERTY(BlueprintReadOnly)
    ECrowdControlType CrowdControlType;
    UPROPERTY(BlueprintReadOnly)
    bool bActive = false;
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UBuff> DominantBuffClass;
    UPROPERTY(NotReplicated)
    UBuff* DominantBuffInstance;
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;
    bool AddNewBuff(UBuff* Source);
    bool RemoveBuff(UBuff* Source);
    bool RefreshBuff(UBuff* Source);

private:
	
    void SetNewDominantBuff(UBuff* NewDominant);
    UBuff* GetLongestBuff();
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCrowdControlCallback, FCrowdControlStatus const&, PreviousStatus, FCrowdControlStatus const&, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCrowdControlNotification, FCrowdControlStatus const&, PreviousStatus, FCrowdControlStatus const&, NewStatus);