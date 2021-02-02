// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraStatLibrary.h"
#include "StatHandler.h"

UStatHandler* USaiyoraStatLibrary::GetStatHandler(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }
    return Target->FindComponentByClass<UStatHandler>();
}
