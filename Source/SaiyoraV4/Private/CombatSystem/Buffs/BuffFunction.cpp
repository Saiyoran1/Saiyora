// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffFunction.h"

void UBuffFunction::SetupBuffFunction(UBuff* BuffRef)
{
    if (!bBuffFunctionInitialized)
    {
        OwningBuff = BuffRef;
        InitializeBuffFunction();
        bBuffFunctionInitialized = true;
    }
}
