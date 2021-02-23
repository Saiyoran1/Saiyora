// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAbility.h"
#include "AbilityHandler.h"

int32 UPlayerAbility::GetCurrentCharges() const
{
    if (GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return Super::GetCurrentCharges();
    }
    return PredictedCharges;
}

bool UPlayerAbility::CheckChargesMet() const
{
    if (GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return Super::CheckChargesMet();
    }
    return PredictedCharges >= GetChargeCost();
}

void UPlayerAbility::CommitCharges(int32 const CastID)
{
    if (GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        Super::CommitCharges(CastID);
        return;
    }
    ChargePredictions.Add(CastID, GetChargeCost());
    RecalculatePredictedCharges();
}

void UPlayerAbility::PurgeOldPredictions()
{
    TArray<int32> IDs;
    ChargePredictions.GenerateKeyArray(IDs);
    for (int32 const ID : IDs)
    {
        if (ID <= AbilityCooldown.LastAckedID)
        {
            ChargePredictions.Remove(ID);
        }
    }
}

void UPlayerAbility::PurgeFailedPrediction(int32 const FailID)
{
    if (ChargePredictions.Remove(FailID) > 0)
    {
        RecalculatePredictedCharges();
    }
}

void UPlayerAbility::RecalculatePredictedCharges()
{
    int32 const PreviousCharges = GetCurrentCharges();
    PredictedCharges = AbilityCooldown.CurrentCharges;
    for (TTuple<int32, int32> const& Prediction : ChargePredictions)
    {
        PredictedCharges = FMath::Clamp(PredictedCharges - Prediction.Value, 0, GetMaxCharges());
    }
    if (PredictedCharges != PreviousCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, PredictedCharges);
    }
}

void UPlayerAbility::OnRep_AbilityCooldown()
{
    PurgeOldPredictions();
    RecalculatePredictedCharges();
}
