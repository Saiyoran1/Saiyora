// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraCombatLibrary.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraPlayerController.h"
#include "GameFramework/GameState.h"
#include "SaiyoraPlaneComponent.h"

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

float USaiyoraCombatLibrary::GetActorPing(AActor const* Actor)
{
    APawn const* ActorAsPawn = Cast<APawn>(Actor);
    if (!IsValid(ActorAsPawn))
    {
        return 0.0f;
    }
    ASaiyoraPlayerController const* Controller = Cast<ASaiyoraPlayerController>(ActorAsPawn->GetController());
    if (!IsValid(Controller))
    {
        return 0.0f;
    }
    return Controller->GetPlayerPing();
}

ESaiyoraPlane USaiyoraCombatLibrary::GetActorPlane(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return ESaiyoraPlane::None;
    }
    if (!Actor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return ESaiyoraPlane::None;
    }
    USaiyoraPlaneComponent* PlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(Actor);
    if (!IsValid(PlaneComponent))
    {
        return ESaiyoraPlane::None;
    }
    return PlaneComponent->GetCurrentPlane();
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

void USaiyoraCombatLibrary::ExtractCombatParameters(FCombatParameters const& Parameters, FVector& OriginLocation,
    FVector& OriginRotation, FVector& OriginScale, FVector& TargetLocation, FVector& TargetRotation,
    FVector& TargetScale, UObject*& Object)
{
    //TODO: Sort Parameters and assign them to the references passed in.
}
