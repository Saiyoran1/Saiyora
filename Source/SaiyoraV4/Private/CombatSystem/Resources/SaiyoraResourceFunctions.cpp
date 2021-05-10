// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraResourceFunctions.h"
#include "SaiyoraCombatInterface.h"
#include "Resource.h"

//TODO: Non-ability resource usage.

void USaiyoraResourceFunctions::ModifyResource(AActor* AppliedTo, TSubclassOf<UResource> const ResourceClass, 
    UObject* Source, float const Amount, bool const bIgnoreModifiers)
{
    if (!IsValid(AppliedTo) || AppliedTo->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    if (!IsValid(ResourceClass))
    {
        return;
    }
    if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    UResourceHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetResourceHandler(AppliedTo);
    if (!IsValid(TargetComponent))
    {
        return;
    }
    UResource* Found = TargetComponent->FindActiveResource(ResourceClass);
    if (!IsValid(Found))
    {
        return;
    }
    Found->ModifyResource(Source, Amount, bIgnoreModifiers);
}
