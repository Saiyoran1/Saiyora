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

    //Returns true if status changed.
    bool AddNewBuff(UBuff* Source);
    //Returns true if status changed.
    bool RemoveBuff(UBuff* Source);
    //Returns true if status changed.
    bool RefreshBuff(UBuff* Source);
private:
    void SetNewDominantBuff(UBuff* NewDominant);
    UBuff* GetLongestBuff();
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCrowdControlCallback, FCrowdControlStatus const&, PreviousStatus,
                                   FCrowdControlStatus const&, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCrowdControlNotification, FCrowdControlStatus const&, PreviousStatus, FCrowdControlStatus const&, NewStatus);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FCrowdControlRestriction, UBuff*, Source, ECrowdControlType const, CrowdControlType);