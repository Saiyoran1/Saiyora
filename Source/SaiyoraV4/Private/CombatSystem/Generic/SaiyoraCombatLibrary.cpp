// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraCombatLibrary.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraPlayerController.h"
#include "GameFramework/GameState.h"
#include "SaiyoraPlaneComponent.h"
#include "Buff.h"
#include "BuffFunction.h"

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

EActorNetPermission USaiyoraCombatLibrary::GetActorNetPermission(AActor const* Actor)
{
    if (!IsValid(Actor))
    {
        return EActorNetPermission::None;
    }
    switch (Actor->GetLocalRole())
    {
        case ROLE_SimulatedProxy :
            return EActorNetPermission::None;
        case ROLE_AutonomousProxy :
            return EActorNetPermission::PredictPlayer;
        case ROLE_Authority :
            {
                if (Actor->GetNetMode() == NM_DedicatedServer)
                {
                    return Actor->GetRemoteRole() == ROLE_AutonomousProxy ? EActorNetPermission::ServerPlayer : EActorNetPermission::Server;
                }
                if (Actor->GetRemoteRole() == ROLE_SimulatedProxy)
                {
                    return EActorNetPermission::Server;
                }
                //TODO: check if this works lol.
                return Actor->HasLocalNetOwner() ? EActorNetPermission::ListenServer : EActorNetPermission::ServerPlayer;
            }
        default :
            return EActorNetPermission::None;
    }
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

FCombatModifier USaiyoraCombatLibrary::MakeCombatModifier(int32& ModifierID, UBuff* Source, EModifierType const ModifierType,
    float const ModifierValue, bool const bStackable)
{
    FCombatModifier OutMod;
    if (ModifierType == EModifierType::Invalid)
    {
        ModifierID = 0;
        return OutMod;
    }
    if (IsValid(Source))
    {
        OutMod.Source = Source;
        OutMod.bFromBuff = true;
    }
    OutMod.ModType = ModifierType;
    OutMod.ModValue = ModifierValue;
    OutMod.bStackable = bStackable;
    OutMod.ID = FCombatModifier::GetID();
    ModifierID = OutMod.ID;
    return OutMod;
}

FCombatModifier USaiyoraCombatLibrary::MakeBuffFunctionCombatModifier(int32& ModifierID, UBuffFunction* Source,
    EModifierType const ModifierType, float const ModifierValue, bool const bStackable)
{
    FCombatModifier OutMod;
    if (ModifierType == EModifierType::Invalid)
    {
        ModifierID = 0;
        return OutMod;
    }
    if (IsValid(Source))
    {
        OutMod.Source = Source->GetOwningBuff();
        OutMod.bFromBuff = true;
    }
    OutMod.ModType = ModifierType;
    OutMod.ModValue = ModifierValue;
    OutMod.bStackable = bStackable;
    OutMod.ID = FCombatModifier::GetID();
    ModifierID = OutMod.ID;
    return OutMod;
}

FCombatModifier USaiyoraCombatLibrary::MakeCombatModifier(EModifierType const ModifierType, float const ModifierValue)
{
    FCombatModifier OutMod;
    if (ModifierType == EModifierType::Invalid)
    {
        return OutMod;
    }
    OutMod.ModType = ModifierType;
    OutMod.ModValue = ModifierValue;
    OutMod.ID = FCombatModifier::GetID();
    return OutMod;
}
