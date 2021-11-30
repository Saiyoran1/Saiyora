#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FStatCallback, FGameplayTag const&, StatTag, float const, NewValue, int32 const, PredictionID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStatNotification, FGameplayTag const&, StatTag, float const, NewValue, int32 const, PredictionID);

USTRUCT()
struct FStatInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Stat", meta = (Categories = "Stat"))
    FGameplayTag StatTag;
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
    bool bShouldReplicate = false;
};