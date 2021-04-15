// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraResourceFunctions.h"
#include "SaiyoraCombatInterface.h"

//TODO: Non-ability resource usage.

void USaiyoraResourceFunctions::ModifyResource(AActor* AppliedTo, FGameplayTag const& ResourceTag, 
    float const Amount, UObject* Source, bool const bIgnoreModifiers)
{
    if (!IsValid(AppliedTo) || AppliedTo->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    if (!ResourceTag.IsValid() || !ResourceTag.MatchesTag(UResourceHandler::GenericResourceTag))
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
    TargetComponent->ModifyResource(ResourceTag, Amount, Source, bIgnoreModifiers);
}
