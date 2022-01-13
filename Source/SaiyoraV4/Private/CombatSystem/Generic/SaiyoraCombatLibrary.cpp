#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerController.h"
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

FCombatModifier USaiyoraCombatLibrary::MakeCombatModifier(UBuff* Source, EModifierType const ModifierType,
    float const ModifierValue, bool const bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source, bStackable);
}

FCombatModifier USaiyoraCombatLibrary::MakeBuffFunctionCombatModifier(UBuffFunction* Source,
    EModifierType const ModifierType, float const ModifierValue, bool const bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source->GetOwningBuff(), bStackable);
}

bool USaiyoraCombatLibrary::ValidatePredictedLineTrace(TArray<FCombatParameter> const& PredictionParams)
{
    //TODO: Line trace sanity checking.
    return true;
}
