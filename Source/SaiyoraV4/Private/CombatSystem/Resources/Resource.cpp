#include "Resource.h"
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
    
    Handler = NewHandler;
    StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(Handler->GetOwner());
    //Set initial values using either the provided init info or the class defaults.
    ResourceState.Maximum = FMath::Max(1.0f, InitInfo.bHasCustomMaximum ? InitInfo.CustomMaxValue : DefaultMaximum);
    ResourceState.CurrentValue = FMath::Clamp(InitInfo.bHasCustomInitial ? InitInfo.CustomInitialValue : DefaultValue, 0.0f, ResourceState.Maximum);
    ResourceState.PredictionID = 0;

    //Bind the resource maximum to a stat if needed.
    if (MaximumBindStat.IsValid() && MaximumBindStat.MatchesTag(FSaiyoraCombatTags::Get().Stat))
    {
        if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(MaximumBindStat))
        {
            MaxStatBind.BindDynamic(this, &UResource::UpdateMaximumFromStatBind);
            StatHandlerRef->SubscribeToStatChanged(MaximumBindStat, MaxStatBind);
            UpdateMaximumFromStatBind(MaximumBindStat, StatHandlerRef->GetStatValue(MaximumBindStat));
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
    ResourceState.CurrentValue = FMath::Clamp(NewValue, 0.0f, ResourceState.Maximum);
    //This function is only called on the server, so we update the prediction ID that last modified this resource.
    //When the client receives this updated ID it can discard its old predictions from before this ID, and recalculate its predicted value.
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

void UResource::UpdateMaximumFromStatBind(const FGameplayTag StatTag, const float NewValue)
{
    if (Handler->GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTagExact(MaximumBindStat) || bDeactivated)
    {
        return;
    }
    const FResourceState PreviousState = ResourceState;
    ResourceState.Maximum = FMath::Max(NewValue, 1.0f);
    //Adjust the current value to accommodate for the new maximum.
    switch (ResourceAdjustmentBehavior)
    {
    case EResourceAdjustmentBehavior::OffsetFromMax :
        {
            const float Offset = PreviousState.Maximum - PreviousState.CurrentValue;
            ResourceState.CurrentValue = ResourceState.Maximum - Offset;
        }
        break;
    case EResourceAdjustmentBehavior::PercentOfMax :
        {
            ResourceState.CurrentValue = (ResourceState.CurrentValue / PreviousState.Maximum) * ResourceState.Maximum;
        }
        break;
    default :
        break;
    }
    ResourceState.CurrentValue = FMath::Clamp(ResourceState.CurrentValue, 0.0f, ResourceState.Maximum);
    
    if (bInitialized && PreviousState.Maximum != ResourceState.Maximum || PreviousState.CurrentValue != ResourceState.CurrentValue)
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
    //Predicting clients can recalculate their predicted value using the server's latest prediction ID and updated state.
    if (Handler->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        PurgeOldPredictions();
        RecalculatePredictedResource(nullptr);
    }
    //Sim proxies can just fire off the OnResourceChangedDelegate.
    else
    {
        if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.CurrentValue != ResourceState.CurrentValue)
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
    //If replication was faster than the server ability ack RPC, we have already recalculated for this prediction ID.
    if (ResourceState.PredictionID >= PredictionID)
    {
        return;
    }
    //Adding to the prediction map will overwrite our original prediction with the server's value.
    ResourcePredictions.Add(PredictionID, ServerCost);
    RecalculatePredictedResource(nullptr);
}

void UResource::RecalculatePredictedResource(UObject* ChangeSource)
{
    const FResourceState PreviousState = FResourceState(ResourceState.Maximum, PredictedResourceValue);
    //Start with the last replicated value from the server.
    PredictedResourceValue = ResourceState.CurrentValue;
    //Apply all predictions in order.
    for (const TTuple<int32, float>& Prediction : ResourcePredictions)
    {
        PredictedResourceValue = FMath::Clamp(PredictedResourceValue - Prediction.Value, 0.0f, ResourceState.Maximum);
    }
    if (PreviousState.CurrentValue != PredictedResourceValue)
    {
        OnResourceChanged.Broadcast(this, ChangeSource, PreviousState, FResourceState(ResourceState.Maximum, PredictedResourceValue));
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