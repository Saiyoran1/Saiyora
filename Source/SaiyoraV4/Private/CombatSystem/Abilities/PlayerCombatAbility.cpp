#include "CombatSystem/Abilities/PlayerCombatAbility.h"
#include "SaiyoraCombatLibrary.h"


void UPlayerCombatAbility::UpdatePredictedChargesFromServer(int32 const PredictionID, int32 const ChargesSpent)
{
    if (AbilityCooldown.PredictionID >= PredictionID)
    {
        return;
    }
    int32 const OldPrediction = ChargePredictions.FindRef(PredictionID);
    if (OldPrediction != ChargesSpent)
    {
        ChargePredictions.Add(PredictionID, ChargesSpent);
        RecalculatePredictedCooldown();
    }
}