// Fill out your copyright notice in the Description page of Project Settings.

#include "Resource.h"
#include "ResourceHandler.h"
#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "CombatAbility.h"

void UResource::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UResource, Handler);
    DOREPLIFETIME(UResource, ResourceState);
    DOREPLIFETIME(UResource, bDeactivated);
}

UWorld* UResource::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UResource::AuthInitializeResource(UResourceHandler* NewHandler, UStatHandler* StatHandler, FResourceInitInfo const& InitInfo)
{
    if (bInitialized || !IsValid(NewHandler) || NewHandler->GetOwnerRole() != ROLE_Authority)
    {
        //Guard against multiple initialization, invalid initialization, and calling this on the client.
        return;
    }
    
    //Set initial values.
    Handler = NewHandler;
    ResourceState.Minimum = FMath::Max(0.0f, InitInfo.bHasCustomMinimum ? InitInfo.CustomMinValue : DefaultMinimum);
    ResourceState.Maximum = FMath::Max(GetMinimum(), InitInfo.bHasCustomMaximum ? InitInfo.CustomMaxValue : DefaultMaximum);
    ResourceState.CurrentValue = FMath::Clamp(InitInfo.bHasCustomInitial ? InitInfo.CustomInitialValue : DefaultValue, GetMinimum(), GetMaximum());
    ResourceState.PredictionID = 0;
    
    //Bind the resource minimum to a stat if needed.
    if (MinimumBindStat.IsValid() && MinimumBindStat.MatchesTag(UStatHandler::GenericStatTag()))
    {
        if (IsValid(StatHandler))
        {
            FStatCallback MinStatBind;
            MinStatBind.BindDynamic(this, &UResource::UpdateMinimumFromStatBind);
            StatHandler->SubscribeToStatChanged(MinimumBindStat, MinStatBind);
            //Manually set the minimum with the initial stat value.
            float const StatValue = StatHandler->GetStatValue(MinimumBindStat);
            if (StatValue >= 0.0f)
            {
                float const PreviousMin = GetMinimum();
                ResourceState.Minimum = FMath::Clamp(StatValue, 0.0f, GetMaximum());
                //Maintain current value relative to the max and min, if the value follows min changes.
                if (bValueFollowsMinimum)
                {
                    ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(PreviousMin, GetMaximum()), FVector2D(GetMinimum(), GetMaximum()), GetCurrentValue());
                }
            }
        }
    }

    //Bind the resource maximum to a stat if needed.
    if (MaximumBindStat.IsValid() && MaximumBindStat.MatchesTag(UStatHandler::GenericStatTag()))
    {
        if (IsValid(StatHandler))
        {
            FStatCallback MaxStatBind;
            MaxStatBind.BindDynamic(this, &UResource::UpdateMaximumFromStatBind);
            StatHandler->SubscribeToStatChanged(MaximumBindStat, MaxStatBind);
            //Manually set the maximum with the initial stat value.
            float const StatValue = StatHandler->GetStatValue(MaximumBindStat);
            if (StatValue >= 0.0f)
            {
                float const PreviousMax = GetMaximum();
                ResourceState.Maximum = FMath::Max(StatValue, GetMinimum());
                //Maintain current value relative to the max and min, if the value follows max changes.
                if (bValueFollowsMaximum)
                {
                    ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(GetMinimum(), PreviousMax), FVector2D(GetMinimum(), GetMaximum()), GetCurrentValue());
                }
            }
        }
    }
    
    bInitialized = true;
    InitializeResource();
}

void UResource::AuthDeactivateResource()
{
    DeactivateResource();
    bDeactivated = true;
}

void UResource::CommitAbilityCost(UCombatAbility* Ability, float const Cost, int32 const PredictionID)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    SetResourceValue(ResourceState.CurrentValue - Cost, Ability, PredictionID);
}

void UResource::PredictAbilityCost(UCombatAbility* Ability, int32 const PredictionID, float const Cost)
{
    ResourcePredictions.Add(PredictionID, Cost);
    RecalculatePredictedResource(Ability);
}

void UResource::RollbackFailedCost(int32 const PredictionID)
{
    if (ResourcePredictions.Remove(PredictionID) > 0)
    {
        RecalculatePredictedResource(nullptr);
    }
}

void UResource::UpdateCostPredictionFromServer(int32 const PredictionID, float const ServerCost)
{
    if (ResourceState.PredictionID >= PredictionID)
    {
        return;
    }
    float const OldPrediction = ResourcePredictions.FindRef(PredictionID);
    if (OldPrediction != ServerCost)
    {
        ResourcePredictions.Add(PredictionID, ServerCost);
        RecalculatePredictedResource(nullptr);
    }
}

void UResource::ModifyResource(UObject* Source, float const Amount, bool const bIgnoreModifiers)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    float ModifiedCost = Amount;
    
    if (!bIgnoreModifiers)
    {
        TArray<FCombatModifier> Mods;
        for (TTuple<int32, FResourceGainModifier>& Mod : ResourceGainMods)
        {
            if (Mod.Value.IsBound())
            {
                Mods.Add(Mod.Value.Execute(this, Source, Amount));
            }
        }
        ModifiedCost = FCombatModifier::ApplyModifiers(Mods, Amount);
    }
    
    SetResourceValue(ResourceState.CurrentValue + ModifiedCost, Source);
}

void UResource::RecalculatePredictedResource(UObject* ChangeSource)
{
    FResourceState const PreviousState = PredictedResourceState;
    PredictedResourceState = ResourceState;
    for (TTuple<int32, float> const& Prediction : ResourcePredictions)
    {
        PredictedResourceState.CurrentValue = FMath::Clamp(PredictedResourceState.CurrentValue - Prediction.Value, ResourceState.Minimum, ResourceState.Maximum);
    }
    if (PreviousState.CurrentValue != PredictedResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, ChangeSource, PreviousState, PredictedResourceState);
        OnResourceUpdated(ChangeSource, PreviousState, PredictedResourceState);
    }
}

void UResource::PurgeOldPredictions()
{
    TArray<int32> OldPredictionIDs;
    for (TTuple<int32, float> const& Prediction : ResourcePredictions)
    {
        if (Prediction.Key <= ResourceState.PredictionID)
        {
            OldPredictionIDs.Add(Prediction.Key);
        }
    }
    for (int32 const ID : OldPredictionIDs)
    {
        ResourcePredictions.Remove(ID);
    }
}

void UResource::InitializeReplicatedResource()
{
    PredictedResourceState = ResourceState;
    InitializeResource();
    bInitialized = true;
}

void UResource::OnRep_Deactivated()
{
    if (bDeactivated)
    {
        DeactivateResource();
        if (IsValid(Handler))
        {
            Handler->NotifyOfRemovedReplicatedResource(this);
        }
    }
}

void UResource::OnRep_Handler()
{
    if (!IsValid(Handler))
    {
        return;
    }
    if (!bReceivedInitialState)
    {
        return;
    }
    if (bInitialized)
    {
        return;
    }
    InitializeReplicatedResource();
    Handler->NotifyOfReplicatedResource(this);
}

void UResource::OnRep_ResourceState(FResourceState const& PreviousState)
{
    if (!bReceivedInitialState)
    {
        bReceivedInitialState = true;
    }
    if (!IsValid(Handler))
    {
        return;
    }
    if (!bInitialized)
    {
        InitializeReplicatedResource();
        Handler->NotifyOfReplicatedResource(this);
        return;
    }
    if (Handler->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.Minimum != ResourceState.Minimum || PreviousState.CurrentValue != ResourceState.CurrentValue)
        {
            OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
            OnResourceUpdated(nullptr, PreviousState, ResourceState);
        }
        return;
    }
    PurgeOldPredictions();
    RecalculatePredictedResource(nullptr);
}

void UResource::SetNewMinimum(float const NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    FResourceState const PreviousState = ResourceState;
    ResourceState.Minimum = FMath::Clamp(NewValue, 0.0f, GetMaximum());
    if (bValueFollowsMinimum)
    {
        ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(PreviousState.Minimum, GetMaximum()), FVector2D(GetMinimum(), GetMaximum()), GetCurrentValue());
    }
    if (PreviousState.Minimum != ResourceState.Minimum || PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
        OnResourceUpdated(nullptr, PreviousState, ResourceState);
    }
}

void UResource::SetNewMaximum(float const NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    FResourceState const PreviousState = ResourceState;
    ResourceState.Maximum = FMath::Max(NewValue, GetMinimum());
    if (bValueFollowsMaximum)
    {
        ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(GetMinimum(), PreviousState.Maximum), FVector2D(GetMinimum(), GetMaximum()), GetCurrentValue());
    }
    if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
        OnResourceUpdated(nullptr, PreviousState, ResourceState);
    }
}

void UResource::SetResourceValue(float const NewValue, UObject* Source, int32 const PredictionID)
{
    FResourceState const PreviousState = ResourceState;
    ResourceState.CurrentValue = FMath::Clamp(NewValue, ResourceState.Minimum, ResourceState.Maximum);
    if (PredictionID != 0)
    {
        ResourceState.PredictionID = PredictionID;
    }
    if (PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, Source, PreviousState, ResourceState);
        OnResourceUpdated(Source, PreviousState, ResourceState);
    }
}

void UResource::UpdateMinimumFromStatBind(FGameplayTag const& StatTag, float const NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    if (StatTag != MinimumBindStat)
    {
        return;
    }
    SetNewMinimum(NewValue);
}

void UResource::UpdateMaximumFromStatBind(FGameplayTag const& StatTag, float const NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    if (StatTag != MaximumBindStat)
    {
        return;
    }
    SetNewMaximum(NewValue);
}

int32 UResource::AddResourceGainModifier(FResourceGainModifier const& Modifier)
{
    if (!Modifier.IsBound())
    {
        return -1;
    }
    int32 const ModID = FModifierCollection::GetID();
    ResourceGainMods.Add(ModID, Modifier);
    return ModID;
}

void UResource::RemoveResourceGainModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    ResourceGainMods.Remove(ModifierID);
}

void UResource::SubscribeToResourceChanged(FResourceValueCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnResourceChanged.AddUnique(Callback);
}

void UResource::UnsubscribeFromResourceChanged(FResourceValueCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnResourceChanged.Remove(Callback);
}

float UResource::GetCurrentValue() const
{
    if (Handler->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return ResourceState.CurrentValue;
    }
    return PredictedResourceState.CurrentValue;
}
