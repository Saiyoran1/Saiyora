// Fill out your copyright notice in the Description page of Project Settings.


#include "ResourceStructs.h"
#include "ResourceHandler.h"

//TODO: Clean up old structs, migrate into new PredictedResource class.
/*void FPredictedResource::PurgeOldPredictions()
{
    if (LastAckedAbilityID == 0)
    {
        return;
    }
    TArray<int32> DeltaIDs;
    PredictedResourceDeltas.GetKeys(DeltaIDs);
    for (int32 ID : DeltaIDs)
    {
        if (ID <= LastAckedAbilityID)
        {
            PredictedResourceDeltas.Remove(ID);
        }
    }
}

float FPredictedResource::UpdatePredictedValue()
{
    float PredictedValue = ServerValue;
    for (TTuple<int32, float> const Delta : PredictedResourceDeltas)
    {
        PredictedValue = FMath::Clamp(PredictedValue + Delta.Value, Minimum, Maximum);
    }
    DisplayValue = PredictedValue;
    return PredictedValue;
}

float FPredictedResource::RollbackPredictedDelta(int32 const CastID)
{
    if (CastID <= 0)
    {
        return DisplayValue;
    }
    TArray<int32> DeltaIDs;
    PredictedResourceDeltas.GetKeys(DeltaIDs);
    for (int32 ID : DeltaIDs)
    {
        if (ID == CastID)
        {
            PredictedResourceDeltas.Remove(ID);
        }
    }
    return UpdatePredictedValue();
}
*/