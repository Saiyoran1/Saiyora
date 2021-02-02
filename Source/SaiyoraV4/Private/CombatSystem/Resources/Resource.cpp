// Fill out your copyright notice in the Description page of Project Settings.


#include "Resource.h"
#include "ResourceHandler.h"
#include "SaiyoraStatLibrary.h"
#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "CombatAbility.h"

void UResource::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UResource, Handler);
    DOREPLIFETIME(UResource, ResourceState);
}

void UResource::AuthInitializeResource(UResourceHandler* NewHandler, FResourceInitInfo const& InitInfo)
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
    ResourceState.LastCastID = 0;
    
    //Bind the resource minimum to a stat if needed.
    if (MinimumBindStat.IsValid() && MinimumBindStat.MatchesTag(UStatHandler::GenericStatTag))
    {
        FStatCallback MinStatBind;
        MinStatBind.BindUFunction(this, FName(TEXT("UpdateMinimumFromStatBind")));
        UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(Handler->GetOwner());
        if (StatHandler)
        {
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
    if (MaximumBindStat.IsValid() && MaximumBindStat.MatchesTag(UStatHandler::GenericStatTag))
    {
        FStatCallback MaxStatBind;
        MaxStatBind.BindUFunction(this, FName(TEXT("UpdateMaximumFromStatBind")));
        UStatHandler* StatHandler = USaiyoraStatLibrary::GetStatHandler(Handler->GetOwner());
        if (StatHandler)
        {
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
    
    InitializeResource();
    bInitialized = true;
}

void UResource::AuthDeactivateResource()
{
    DeactivateResource();
    bDeactivated = true;
}

bool UResource::CalculateAndCheckAbilityCost(UCombatAbility* Ability, FAbilityCost& Cost)
{
    if (!Cost.bStaticCost)
    {
        FCombatModifier TempMod;
        TArray<FCombatModifier> ValidMods;
        for (FResourceDeltaModifier const& Condition : ResourceDeltaMods)
        {
            TempMod = Condition.Execute(GetResourceTag(), Ability, Cost);
            if (TempMod.ModifierType != EModifierType::Invalid)
            {
                ValidMods.Add(TempMod);
            }
        }
        float AddMod = 0.0f;
        float MultMod = 1.0f;
        for (FCombatModifier const& Mod : ValidMods)
        {
            switch (Mod.ModifierType)
            {
            case EModifierType::Invalid :
                break;
            case EModifierType::Additive :
                AddMod += Mod.ModifierValue;
                break;
            case EModifierType::Multiplicative :
                MultMod *= Mod.ModifierValue;
                break;
            default :
                break;
            }
        }
        Cost.Cost = (Cost.Cost + AddMod) * MultMod;
    }
    if (Cost.Cost <= GetCurrentValue())
    {
        return true;
    }
    return false;
}

void UResource::CommitAbilityCost(UCombatAbility* Ability, int32 const CastID, FAbilityCost const& Cost)
{
    if (Handler->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    FResourceState const PreviousState = ResourceState;
    ResourceState.CurrentValue = FMath::Clamp(GetCurrentValue() - Cost.Cost, ResourceState.Minimum, ResourceState.Maximum);
    ResourceState.LastCastID = CastID;
    if (PreviousState.CurrentValue != GetCurrentValue() || PreviousState.LastCastID != ResourceState.LastCastID)
    {
        OnResourceChanged.Broadcast(GetResourceTag(), Ability, PreviousState, ResourceState);
    }
}

void UResource::InitializeReplicatedResource()
{
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
            Handler->NotifyOfRemovedReplicatedResource(ResourceTag);
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
    if (PreviousState.Maximum != ResourceState.Maximum || PreviousState.Minimum != ResourceState.Minimum || PreviousState.CurrentValue != ResourceState.CurrentValue)
    {
        OnResourceChanged.Broadcast(GetResourceTag(), nullptr, PreviousState, ResourceState);
    }
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
    if (PreviousState.Minimum != GetMinimum() || PreviousState.CurrentValue != GetCurrentValue())
    {
        OnResourceChanged.Broadcast(ResourceTag, nullptr, PreviousState, ResourceState);
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
    if (PreviousState.Maximum != GetMaximum() || PreviousState.CurrentValue != GetCurrentValue())
    {
        OnResourceChanged.Broadcast(ResourceTag, nullptr, PreviousState, ResourceState);
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

void UResource::AddResourceDeltaModifier(FResourceDeltaModifier const& Modifier)
{
    if (!Modifier.IsBound())
    {
        return;
    }
    int32 const PreviousLength = ResourceDeltaMods.Num();
    ResourceDeltaMods.AddUnique(Modifier);
    if (PreviousLength != ResourceDeltaMods.Num())
    {
        OnResourceDeltaModsChanged.Broadcast(ResourceTag);
    }
}

void UResource::RemoveResourceDeltaModifier(FResourceDeltaModifier const& Modifier)
{
    if (!Modifier.IsBound())
    {
        return;
    }
    if (ResourceDeltaMods.RemoveSingleSwap(Modifier) != 0)
    {
        OnResourceDeltaModsChanged.Broadcast(ResourceTag);
    }
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

void UResource::SubscribeToResourceModsChanged(FResourceTagCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnResourceDeltaModsChanged.AddUnique(Callback);
}

void UResource::UnsubscribeFromResourceModsChanged(FResourceTagCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnResourceDeltaModsChanged.Remove(Callback);
}