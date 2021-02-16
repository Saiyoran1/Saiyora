// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraCrowdControlFunctions.h"
#include "CrowdControlHandler.h"

UCrowdControlHandler* USaiyoraCrowdControlFunctions::GetCrowdControlHandler(AActor* Target)
{
    if (!IsValid(Target))
    {
        return nullptr;
    }
    return Target->FindComponentByClass<UCrowdControlHandler>();
}
