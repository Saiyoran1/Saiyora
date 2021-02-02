// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraCombatLibrary.h"
#include "GameFramework/GameState.h"

//Plane

float USaiyoraCombatLibrary::GetWorldTime(UObject const* Context)
{
    if (Context)
    {
        UWorld* World = Context->GetWorld();
        if (World)
        {
            AGameStateBase* GameState = World->GetGameState();
            if (GameState)
            {
                return GameState->GetServerWorldTimeSeconds();
            }
        }
    }
    return 0.0f;
}

ESaiyoraPlane USaiyoraCombatLibrary::GetActorPlane(AActor* Actor)
{
    //TODO Plane Component
    if (!Actor)
    {
        return ESaiyoraPlane::None;
    }
    return ESaiyoraPlane::None;
}

bool USaiyoraCombatLibrary::CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane)
{
    //None is the default value, always return false if it is provided.
    if (FromPlane == ESaiyoraPlane::None || ToPlane == ESaiyoraPlane::None)
    {
        return false;
    }
    //Actors "in between" planes will see everything as another plane.
    if (FromPlane == ESaiyoraPlane::Neither || ToPlane == ESaiyoraPlane::Neither)
    {
        return true;
    }
    //Actors in both planes will see everything except "in between" actors as the same plane.
    if (FromPlane == ESaiyoraPlane::Both || ToPlane == ESaiyoraPlane::Both)
    {
        return false;
    }
    //Actors in a normal plane will only see actors in the same plane or both planes as the same plane.
    return FromPlane != ToPlane;
}


