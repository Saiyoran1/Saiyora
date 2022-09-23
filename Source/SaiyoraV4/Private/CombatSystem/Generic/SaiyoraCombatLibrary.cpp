#include "SaiyoraCombatLibrary.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "BuffFunction.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

float USaiyoraCombatLibrary::GetActorPing(const AActor* Actor)
{
    APawn const* ActorAsPawn = Cast<APawn>(Actor);
    if (!IsValid(ActorAsPawn))
    {
        return 0.0f;
    }
    const ASaiyoraPlayerController* Controller = Cast<ASaiyoraPlayerController>(ActorAsPawn->GetController());
    if (!IsValid(Controller))
    {
        return 0.0f;
    }
    return Controller->GetPlayerPing();
}

void USaiyoraCombatLibrary::AttachCombatActorToComponent(AActor* Target, USceneComponent* Parent, const FName SocketName,
    const EAttachmentRule LocationRule, const EAttachmentRule RotationRule, const EAttachmentRule ScaleRule, const bool bWeldSimulatedBodies)
{
    if (!IsValid(Target) || !IsValid(Parent))
    {
        return;
    }
    Target->AttachToComponent(Parent, FAttachmentTransformRules(LocationRule, RotationRule, ScaleRule, bWeldSimulatedBodies), SocketName);
    AActor* ParentActor = Parent->GetOwner();
    if (ParentActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        UCombatStatusComponent* CombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(ParentActor);
        if (IsValid(CombatStatusComponent))
        {
            CombatStatusComponent->RefreshRendering();
        }
    }
}
