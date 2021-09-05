// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
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