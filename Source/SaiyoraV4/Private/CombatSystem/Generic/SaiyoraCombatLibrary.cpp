#include "SaiyoraCombatLibrary.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

int USaiyoraCombatLibrary::UniqueCombatEventID = 0;

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
    if (ParentActor->Implements<USaiyoraCombatInterface>())
    {
        UCombatStatusComponent* CombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(ParentActor);
        if (IsValid(CombatStatusComponent))
        {
            CombatStatusComponent->RefreshRendering();
        }
    }
}

TArray<FName> USaiyoraCombatLibrary::GetMatchingFunctionNames(const UObject* Object, const UFunction* ExampleFunction)
{
    TArray<FName> FunctionNames;
    if (!IsValid(Object))
    {
        return FunctionNames;
    }
    if (!ExampleFunction)
    {
        return FunctionNames;
    }
    for (TFieldIterator<UFunction> FunctionIt (Object->GetClass()); FunctionIt; ++FunctionIt)
    {
        const UFunction* Function = *FunctionIt;
        // Early out if they're exactly the same function
        if (Function == ExampleFunction)
        {
            continue;
        }
        if (IsSignatureTheSame(Function, ExampleFunction))
        {
            FunctionNames.Add((*FunctionIt)->GetFName());
        }
    }
    return FunctionNames;
}

bool USaiyoraCombatLibrary::IsSignatureTheSame(const UFunction* Function, const UFunction* Example)
{
    // Run through the parameter property chains to compare each property
    TFieldIterator<FProperty> IteratorA(Function);
    TFieldIterator<FProperty> IteratorB(Example);

    while (IteratorA && (IteratorA->PropertyFlags & CPF_Parm))
    {
        if (IteratorB && (IteratorB->PropertyFlags & CPF_Parm))
        {
            // Compare the two properties to make sure their types are identical
            FProperty* PropA = *IteratorA;
            FProperty* PropB = *IteratorB;
                
            if (!FStructUtils::ArePropertiesTheSame(PropA, PropB, false))
            {
                return false;
            }
        }
        else
        {
            // B ran out of arguments before A did
            return false;
        }
        ++IteratorA;
        ++IteratorB;
    }

    // They matched all the way through A's properties, but it could still be a mismatch if B has remaining parameters
    return !(IteratorB && (IteratorB->PropertyFlags & CPF_Parm));
}

FString USaiyoraCombatLibrary::DelegateToLogString(const FScriptDelegate& Delegate)
{
    return FString::Printf(L"{ Object: %s, Function: %s }",
        IsValid(Delegate.GetUObject()) ? *Delegate.GetUObject()->GetName() : TEXT("NULL"),
        *Delegate.GetFunctionName().ToString());
}

ASaiyoraPlayerCharacter* USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(const UObject* WorldContext)
{
    if (!IsValid(WorldContext) || !IsValid(WorldContext->GetWorld()))
    {
        return nullptr;
    }
    if (!IsValid(WorldContext->GetWorld()->GetGameState()))
    {
        return nullptr;
    }
    for (TObjectPtr<APlayerState> PlayerState : WorldContext->GetWorld()->GetGameState()->PlayerArray)
    {
        if (IsValid(PlayerState.Get()) && IsValid(PlayerState.Get()->GetPlayerController())
            && PlayerState.Get()->GetPlayerController()->IsLocalController() && IsValid(PlayerState.Get()->GetPlayerController()->GetPawn()))
        {
            return Cast<ASaiyoraPlayerCharacter>(PlayerState.Get()->GetPlayerController()->GetPawn());
        }
    }
    return nullptr;
}