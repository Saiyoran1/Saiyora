#include "Resource.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "CombatAbility.h"
#include "SaiyoraCombatInterface.h"

#pragma region Initialization and Deactivation

UWorld* UResource::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UResource::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UResource, Handler);
    DOREPLIFETIME(UResource, bDeactivated);
    DOREPLIFETIME(UResource, ResourceState);
}

void UResource::PostNetReceive()
{
    if (bInitialized || bDeactivated)
    {
        return;
    }
    if (IsValid(Handler))
    {
        if (Handler->GetOwnerRole() == ROLE_AutonomousProxy)
        {
            PredictedResourceValue = ResourceState.CurrentValue;
        }
        PreInitializeResource();
        bInitialized = true;
        Handler->NotifyOfReplicatedResource(this);
    }
}

void UResource::InitializeResource(UResourceHandler* NewHandler, const FResourceInitInfo& InitInfo)
{
    if (bInitialized || !IsValid(NewHandler) || NewHandler->GetOwnerRole() != ROLE_Authority)
    {
        //Guard against multiple initialization, invalid initialization, and calling this on the client.
        return;
    }
    
    //Set initial values.
    Handler = NewHandler;
    StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(Handler->GetOwner());
    ResourceState.Minimum = FMath::Max(0.0f, InitInfo.bHasCustomMinimum ? InitInfo.CustomMinValue : DefaultMinimum);
    ResourceState.Maximum = FMath::Max(ResourceState.Minimum, InitInfo.bHasCustomMaximum ? InitInfo.CustomMaxValue : DefaultMaximum);
    ResourceState.CurrentValue = FMath::Clamp(InitInfo.bHasCustomInitial ? InitInfo.CustomInitialValue : DefaultValue, ResourceState.Minimum, ResourceState.Maximum);
    ResourceState.PredictionID = 0;
    
    //Bind the resource minimum to a stat if needed.
    if (MinimumBindStat.IsValid() && MinimumBindStat.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !MinimumBindStat.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
    {
        if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(MinimumBindStat))
        {
            MinStatBind.BindDynamic(this, &UResource::UpdateMinimumFromStatBind);
            StatHandlerRef->SubscribeToStatChanged(MinimumBindStat, MinStatBind);
            //Manually set the minimum with the initial stat value.
            if (const float StatValue = StatHandlerRef->GetStatValue(MinimumBindStat) >= 0.0f)
            {
                const float PreviousMin = ResourceState.Minimum;
                ResourceState.Minimum = FMath::Clamp(StatValue, 0.0f, ResourceState.Maximum);
                //Maintain current value relative to the max and min, if the value follows min changes.
                ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(PreviousMin, ResourceState.Maximum), FVector2D(ResourceState.Minimum, ResourceState.Maximum), ResourceState.CurrentValue);
            }
        }
    }

    //Bind the resource maximum to a stat if needed.
    if (MaximumBindStat.IsValid() && MaximumBindStat.MatchesTag(FSaiyoraCombatTags::Get().Stat))
    {
        if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(MaximumBindStat))
        {
            MaxStatBind.BindDynamic(this, &UResource::UpdateMaximumFromStatBind);
            StatHandlerRef->SubscribeToStatChanged(MaximumBindStat, MaxStatBind);
            //Manually set the maximum with the initial stat value.
            if (const float StatValue = StatHandlerRef->GetStatValue(MaximumBindStat) >= 0.0f)
            {
                const float PreviousMax = GetMaximum();
                ResourceState.Maximum = FMath::Max(StatValue, GetMinimum());
                //Maintain current value relative to the max and min, if the value follows max changes.
                ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(ResourceState.Minimum, PreviousMax), FVector2D(ResourceState.Minimum, ResourceState.Maximum), ResourceState.CurrentValue);
            }
        }
    }
    
    PreInitializeResource();
    bInitialized = true;
}

void UResource::DeactivateResource()
{
    PreDeactivateResource();
    OnResourceChanged.Clear();
    bDeactivated = true;
}

void UResource::OnRep_Deactivated()
{
    //Don't bother calling deactivate if we were never initialized.
    if (bDeactivated && bInitialized)
    {
        DeactivateResource();
        if (IsValid(Handler))
        {
            Handler->NotifyOfRemovedReplicatedResource(this);
        }
    }
}

#pragma endregion
#pragma region State

float UResource::GetCurrentValue() const
{
    return Handler->GetOwnerRole() == ROLE_AutonomousProxy ? PredictedResourceValue : ResourceState.CurrentValue;
}

void UResource::ModifyResource(UObject* Source, const float Amount, const bool bIgnoreModifiers)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority || !bInitialized || bDeactivated)
    {
        return;
    }
    float Delta = Amount;
    if (!bIgnoreModifiers)
    {
        TArray<FCombatModifier> Mods;
        ResourceDeltaMods.GetModifiers(Mods, this, Source, Amount);
        Delta = FCombatModifier::ApplyModifiers(Mods, Delta);
    }
    SetResourceValue(ResourceState.CurrentValue + Delta, Source);
}

void UResource::SetResourceValue(const float NewValue, UObject* Source, const int32 PredictionID)
{
    if (!bInitialized || bDeactivated)
    {
        return;
    }
    const FResourceState PreviousState = ResourceState;
    ResourceState.CurrentValue = FMath::Clamp(NewValue, ResourceState.Minimum, ResourceState.Maximum);
    if (PredictionID != 0)
    {
        ResourceState.PredictionID = PredictionID;
    }
    if (PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, Source, PreviousState, ResourceState);
        PostResourceUpdated(Source, PreviousState);
    }
}

void UResource::UpdateMinimumFromStatBind(const FGameplayTag StatTag, const float NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTagExact(MinimumBindStat) || !bInitialized || bDeactivated)
    {
        return;
    }
    const FResourceState PreviousState = ResourceState;
    ResourceState.Minimum = FMath::Clamp(NewValue, 0.0f, ResourceState.Maximum);
    ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(PreviousState.Minimum, ResourceState.Maximum), FVector2D(ResourceState.Minimum, ResourceState.Maximum), ResourceState.CurrentValue);
    if (PreviousState.Minimum != ResourceState.Minimum || PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
        PostResourceUpdated(nullptr, PreviousState);
    }
}

void UResource::UpdateMaximumFromStatBind(const FGameplayTag StatTag, const float NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTagExact(MaximumBindStat) || !bInitialized || bDeactivated)
    {
        return;
    }
    const FResourceState PreviousState = ResourceState;
    ResourceState.Maximum = FMath::Max(NewValue, GetMinimum());
    ResourceState.CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(GetMinimum(), PreviousState.Maximum), FVector2D(GetMinimum(), GetMaximum()), GetCurrentValue());
    if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
        PostResourceUpdated(nullptr, PreviousState);
    }
}

void UResource::OnRep_ResourceState(const FResourceState& PreviousState)
{
    if (!bInitialized || bDeactivated || !IsValid(Handler))
    {
        return;
    }
    if (Handler->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        PurgeOldPredictions();
        RecalculatePredictedResource(nullptr);
    }
    else
    {
        if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.Minimum != ResourceState.Minimum || PreviousState.CurrentValue != ResourceState.CurrentValue)
        {
            OnResourceChanged.Broadcast(this, nullptr, PreviousState, ResourceState);
            PostResourceUpdated(nullptr, PreviousState);
        }
    }
}

#pragma endregion
#pragma region Ability Costs

void UResource::CommitAbilityCost(UCombatAbility* Ability, const float Cost, const int32 PredictionID)
{
    if (Handler->GetOwnerRole() == ROLE_Authority)
    {
        SetResourceValue(ResourceState.CurrentValue - Cost, Ability, PredictionID);
    }
    else if (Handler->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        ResourcePredictions.Add(PredictionID, Cost);
        RecalculatePredictedResource(Ability);
    }
}

void UResource::UpdateCostPredictionFromServer(const int32 PredictionID, const float ServerCost)
{
    if (ResourceState.PredictionID >= PredictionID)
    {
        return;
    }
    ResourcePredictions.Add(PredictionID, ServerCost);
    RecalculatePredictedResource(nullptr);
}

void UResource::RecalculatePredictedResource(UObject* ChangeSource)
{
    const FResourceState PreviousState = FResourceState(ResourceState.Minimum, ResourceState.Maximum, PredictedResourceValue);
    PredictedResourceValue = ResourceState.CurrentValue;
    for (const TTuple<int32, float>& Prediction : ResourcePredictions)
    {
        PredictedResourceValue = FMath::Clamp(PredictedResourceValue - Prediction.Value, ResourceState.Minimum, ResourceState.Maximum);
    }
    if (PreviousState.CurrentValue != PredictedResourceValue)
    {
        OnResourceChanged.Broadcast(this, ChangeSource, PreviousState, FResourceState(ResourceState.Minimum, ResourceState.Maximum, PredictedResourceValue));
        PostResourceUpdated(ChangeSource, PreviousState);
    }
}

void UResource::PurgeOldPredictions()
{
    TArray<int32> Keys;
    ResourcePredictions.GetKeys(Keys);
    for (const int32 ID : Keys)
    {
        if (ID <= ResourceState.PredictionID)
        {
            ResourcePredictions.Remove(ID);
        }
    }
}

#pragma endregion 