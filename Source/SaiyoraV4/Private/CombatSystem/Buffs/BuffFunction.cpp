// Fill out your copyright notice in the Description page of Project Settings.
#include "BuffFunction.h"
#include "Buff.h"

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, TSubclassOf<UBuffFunction> const FunctionClass)
{
    if (!IsValid(Buff) || !IsValid(FunctionClass))
    {
        return nullptr;
    }
    UBuffFunction* NewFunction = NewObject<UBuffFunction>(Buff, FunctionClass);
    NewFunction->InitializeBuffFunction(Buff);
    return NewFunction;
}

UWorld* UBuffFunction::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
    if (!bBuffFunctionInitialized)
    {
        if (!IsValid(BuffRef))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid owning buff provided to buff function."));
            return;
        }
        OwningBuff = BuffRef;
        OwningBuff->RegisterBuffFunction(this);
        bBuffFunctionInitialized = true;
        SetupBuffFunction();
    }
}

void UCustomBuffFunction::SetupCustomBuffFunction(UBuff* Buff, TSubclassOf<UCustomBuffFunction> const FunctionClass)
{
    InstantiateBuffFunction(Buff, FunctionClass);
}

void UCustomBuffFunction::SetupCustomBuffFunctions(UBuff* Buff,
    TArray<TSubclassOf<UCustomBuffFunction>> const& FunctionClasses)
{
    for (TSubclassOf<UCustomBuffFunction> const& FunctionClass : FunctionClasses)
    {
        InstantiateBuffFunction(Buff, FunctionClass);
    }
}
