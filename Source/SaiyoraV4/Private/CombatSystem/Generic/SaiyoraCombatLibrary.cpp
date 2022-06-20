#include "SaiyoraCombatLibrary.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "BuffFunction.h"
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

FCombatModifier USaiyoraCombatLibrary::MakeCombatModifier(UBuff* Source, const EModifierType ModifierType,
    const float ModifierValue, const bool bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source, bStackable);
}

FCombatModifier USaiyoraCombatLibrary::MakeBuffFunctionCombatModifier(const UBuffFunction* Source,
    const EModifierType ModifierType, const float ModifierValue, const bool bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source->GetOwningBuff(), bStackable);
}