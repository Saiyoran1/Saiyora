// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "CrowdControlStructs.h"
#include "CrowdControl.generated.h"

class UCrowdControlHandler;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UCrowdControl : public UObject
{
	GENERATED_BODY()

private:

    UPROPERTY()
    TArray<UBuff*> CrowdControls;
    bool bActive;
    UPROPERTY()
    UBuff* DominantBuff;

    UPROPERTY()
    UCrowdControlHandler* Handler;
    bool bInitialized = false;

    UBuff* FindNewDominantBuff();

    TSubclassOf<UBuff> ReplicatedBuffClass;
    float ReplicatedEndTime;

public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
    UCrowdControlHandler* GetHandler() const { return Handler; }

    void InitializeCrowdControl(UCrowdControlHandler* OwningComponent);

    FCrowdControlStatus GetCurrentStatus() const;
    bool GetInitialized() const { return bInitialized; }

    FCrowdControlStatus AddCrowdControl(UBuff* Source);
    FCrowdControlStatus RemoveCrowdControl(UBuff* Source);
    FCrowdControlStatus UpdateCrowdControl(UBuff* RefreshedBuff);

    void RemoveNewlyRestrictedCrowdControls(FCrowdControlRestriction const& Restriction);

    FCrowdControlStatus UpdateCrowdControlFromReplication(bool const Active, TSubclassOf<UBuff> const SourceClass, float const EndTime);

protected:

    UFUNCTION(BlueprintImplementableEvent)
    void OnActivation();
    UFUNCTION(BlueprintImplementableEvent)
    void OnRemoval();
};
