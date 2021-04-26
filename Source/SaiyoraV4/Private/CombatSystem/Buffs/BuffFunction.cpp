// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffFunction.h"

UWorld* UBuffFunction::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UBuffFunction::SetupBuffFunction(UBuff* BuffRef)
{
    if (!bBuffFunctionInitialized)
    {
        OwningBuff = BuffRef;
        InitializeBuffFunction();
        bBuffFunctionInitialized = true;
    }
}
