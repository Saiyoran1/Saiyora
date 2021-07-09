// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatAbility.h"
#include "AbilityHandler.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

void UCombatAbility::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatAbility, OwningComponent);
    DOREPLIFETIME(UCombatAbility, bDeactivated);
    DOREPLIFETIME_CONDITION(UCombatAbility, AbilityCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, MaxCharges, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, ChargesPerCast, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, ReplicatedCosts, COND_OwnerOnly);
}

UWorld* UCombatAbility::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

int32 UCombatAbility::GetCurrentCharges() const
{
    if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return PredictedCooldown.CurrentCharges;
    }
    return AbilityCooldown.CurrentCharges;
}

bool UCombatAbility::CheckChargesMet() const
{
    if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return PredictedCooldown.CurrentCharges >= ChargesPerCast;
    }
    return AbilityCooldown.CurrentCharges >= ChargesPerCast;
}

float UCombatAbility::GetRemainingCooldown() const
{
    switch (OwningComponent->GetOwnerRole())
    {
        case ROLE_Authority :
            return AbilityCooldown.OnCooldown ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f;
        case ROLE_AutonomousProxy :
            return (PredictedCooldown.OnCooldown && PredictedCooldown.CooldownEndTime != 0.0f) ?
                PredictedCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds() : 0.0f;
        case ROLE_SimulatedProxy :
            return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds() : 0.0f;
        default :
            return 0.0f;
    }
}

float UCombatAbility::GetCurrentCooldownLength() const
{
    switch (OwningComponent->GetOwnerRole())
    {
    case ROLE_Authority :
        return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime : 0.0f;
    case ROLE_AutonomousProxy :
        return (PredictedCooldown.OnCooldown && PredictedCooldown.CooldownEndTime != 0.0f) ?
            PredictedCooldown.CooldownEndTime - PredictedCooldown.CooldownStartTime : 0.0f;
    case ROLE_SimulatedProxy :
        return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime : 0.0f;
    default :
        return 0.0f;
    }
}

bool UCombatAbility::GetCooldownActive() const
{
    return OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy ? PredictedCooldown.OnCooldown : AbilityCooldown.OnCooldown;
}

void UCombatAbility::CommitCharges(int32 const PredictionID)
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.CurrentCharges = FMath::Clamp((PreviousCharges - ChargesPerCast), 0, MaxCharges);
    bool bFromPrediction = false;
    if (PredictionID != 0)
    {
        AbilityCooldown.PredictionID = PredictionID;
        bFromPrediction = true;
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < MaxCharges)
    {
        StartCooldown(bFromPrediction);
    }
}

void UCombatAbility::PredictCommitCharges(int32 const PredictionID)
{
    ChargePredictions.Add(PredictionID, ChargesPerCast);
    RecalculatePredictedCooldown();
}

void UCombatAbility::RollbackFailedCharges(int32 const PredictionID)
{
    if (ChargePredictions.Remove(PredictionID) > 0)
    {
        RecalculatePredictedCooldown();   
    }
}

void UCombatAbility::UpdatePredictedChargesFromServer(FServerAbilityResult const& ServerResult)
{
    if (AbilityCooldown.PredictionID >= ServerResult.PredictionID)
    {
        return;
    }
    if (!ServerResult.bSpentCharges || ServerResult.ChargesSpent == 0)
    {
        RollbackFailedCharges(ServerResult.PredictionID);
    }
    else
    {
        int32 const OldPrediction = ChargePredictions.FindRef(ServerResult.PredictionID);
        if (OldPrediction != ServerResult.ChargesSpent)
        {
            ChargePredictions.Add(ServerResult.PredictionID, ServerResult.ChargesSpent);
            RecalculatePredictedCooldown();
        }
    }
}

bool UCombatAbility::GetTickNeedsPredictionParams(int32 const TickNumber) const
{
    return TicksWithPredictionParams.Contains(TickNumber);
}

float UCombatAbility::GetAbilityCost(TSubclassOf<UResource> const ResourceClass) const
{
    if (!IsValid(ResourceClass))
    {
        return -1.0f;
    }
    FAbilityCost const* Cost = AbilityCosts.Find(ResourceClass);
    if (Cost)
    {
        return Cost->Cost;
    }
    return -1.0f;
}

void UCombatAbility::RecalculateAbilityCost(TSubclassOf<UResource> const ResourceClass)
{
    if (!IsValid(ResourceClass))
    {
        return;
    }
    FAbilityCost MutableCost;
    bool bFoundCost = false;
    for (FAbilityCost const& Cost : DefaultAbilityCosts)
    {
        if (Cost.ResourceClass == ResourceClass)
        {
            MutableCost = Cost;
            bFoundCost = true;
            break;
        }
    }
    if (!bFoundCost)
    {
        return;
    }
    if (MutableCost.bStaticCost)
    {
        AbilityCosts.Add(ResourceClass, MutableCost);
        ReplicatedCosts.UpdateAbilityCost(MutableCost);
        return;
    }
    TArray<FCombatModifier> RelevantMods;
    CostModifiers.MultiFind(ResourceClass, RelevantMods);
    MutableCost.Cost = FMath::Max(0.0f, FCombatModifier::CombineModifiers(RelevantMods, MutableCost.Cost));
    AbilityCosts.Add(ResourceClass, MutableCost);
    ReplicatedCosts.UpdateAbilityCost(MutableCost);
}

void UCombatAbility::PurgeOldPredictions()
{
    TArray<int32> OldPredictionIDs;
    for (TTuple <int32, int32> const& Prediction : ChargePredictions)
    {
        if (Prediction.Key <= AbilityCooldown.PredictionID)
        {
            OldPredictionIDs.AddUnique(Prediction.Key);
        }
    }
    for (int32 const ID : OldPredictionIDs)
    {
        ChargePredictions.Remove(ID);
    }
}

void UCombatAbility::RecalculatePredictedCooldown()
{
    FAbilityCooldown const PreviousState = PredictedCooldown;
    PredictedCooldown = AbilityCooldown;
    for (TTuple<int32, int32> const& Prediction : ChargePredictions)
    {
        PredictedCooldown.CurrentCharges = FMath::Clamp(PredictedCooldown.CurrentCharges - Prediction.Value, 0, MaxCharges);
    }
    //We accept authority cooldown progress if server says we are on cd.
    //If server is not on CD, but predicted charges are less than max, we predict a CD has started but do not predict start/end time.
    if (!PredictedCooldown.OnCooldown && PredictedCooldown.CurrentCharges < MaxCharges)
    {
        PredictedCooldown.OnCooldown = true;
        PredictedCooldown.CooldownStartTime = 0.0f;
        PredictedCooldown.CooldownEndTime = 0.0f;
    }
    if (PreviousState.CurrentCharges != PredictedCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousState.CurrentCharges, PredictedCooldown.CurrentCharges);
    }
}

void UCombatAbility::StartCooldown(bool const bFromPrediction)
{
    float CooldownLength = GetHandler()->CalculateAbilityCooldown(this);
    if (bFromPrediction)
    {
        float const PingCompensation = bFromPrediction ? FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetHandler()->GetOwner()), 0.0f, UAbilityHandler::MaxPingCompensation) : 0.0f;
        CooldownLength = FMath::Max(UAbilityHandler::MinimumCooldownLength, CooldownLength - PingCompensation);
    }
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, CooldownLength, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + CooldownLength;
}

void UCombatAbility::CompleteCooldown()
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.CurrentCharges = FMath::Clamp(GetCurrentCharges() + ChargesPerCooldown, 0, GetMaxCharges());
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (AbilityCooldown.CurrentCharges < GetMaxCharges())
    {
        StartCooldown(false);
    }
    else
    {
        AbilityCooldown.OnCooldown = false;
    }
}

void UCombatAbility::GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const
{
    AbilityCosts.GenerateValueArray(OutCosts);
}

void UCombatAbility::InitializeAbility(UAbilityHandler* AbilityComponent)
{
    if (bInitialized)
    {
        return;
    }
    if (IsValid(AbilityComponent))
    {
        OwningComponent = AbilityComponent;
    }
    MaxCharges = DefaultMaxCharges;
    AbilityCooldown.CurrentCharges = GetMaxCharges();
    ChargesPerCast = DefaultChargesPerCast;
    ChargesPerCooldown = DefaultChargesPerCooldown;
    ReplicatedCosts.Ability = this;
    for (FAbilityCost const& Cost : DefaultAbilityCosts)
    {
        AbilityCosts.Add(Cost.ResourceClass, Cost);
        if (AbilityComponent->GetOwnerRole() == ROLE_Authority)
        {
            FReplicatedAbilityCost ReplicatedCost;
            ReplicatedCost.AbilityCost = Cost;
            ReplicatedCosts.MarkItemDirty(ReplicatedCosts.Items.Add_GetRef(ReplicatedCost));
        }
    }
    OnInitialize();
    SetupCustomCastRestrictions();
    bInitialized = true;
}

void UCombatAbility::DeactivateAbility()
{
    if (GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
    }
    OnDeactivate();
    bDeactivated = true;
}

void UCombatAbility::PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return;
    }
    OnPredictedTick(TickNumber, PredictionParams.Parameters);
}

void UCombatAbility::ServerPredictedTick(int32 const TickNumber, FCombatParameters const& PredictionParams,
    FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerPredictedTick(TickNumber, PredictionParams.Parameters, BroadcastParams.Parameters);
}

void UCombatAbility::ServerTick(int32 const TickNumber, FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerTick(TickNumber, BroadcastParams.Parameters);
}

void UCombatAbility::SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        return;
    }
    OnSimulatedTick(TickNumber, BroadcastParams.Parameters);
}

void UCombatAbility::CompleteCast()
{
    OnCastComplete();
}

void UCombatAbility::InterruptCast(FInterruptEvent const& InterruptEvent)
{
    OnCastInterrupted(InterruptEvent);
}

void UCombatAbility::PredictedCancel(FCombatParameters& PredictionParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return;
    }
    OnPredictedCancel(PredictionParams.Parameters);
}

void UCombatAbility::ServerPredictedCancel(FCombatParameters const& PredictionParams,
    FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerPredictedCancel(PredictionParams.Parameters, BroadcastParams.Parameters);
}

void UCombatAbility::ServerCancel(FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerCancel(BroadcastParams.Parameters);
}

void UCombatAbility::SimulatedCancel(FCombatParameters const& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        return;
    }
    OnSimulatedCancel(BroadcastParams.Parameters);
}

void UCombatAbility::AbilityMisprediction(int32 const PredictionID, FString const& FailReason)
{
    OnAbilityMispredicted(PredictionID, FailReason);
}

void UCombatAbility::SubscribeToChargesChanged(FAbilityChargeCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnChargesChanged.AddUnique(Callback);
    }
}

void UCombatAbility::UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnChargesChanged.Remove(Callback);
    }
}

void UCombatAbility::AddAbilityCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    if (Modifier.ModType == EModifierType::Invalid)
    {
        return;
    }
    if (!IsValid(ResourceClass))
    {
        return;   
    }
    CostModifiers.Add(ResourceClass, Modifier);
    RecalculateAbilityCost(ResourceClass);
}

void UCombatAbility::RemoveAbilityCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    if (!IsValid(ResourceClass))
    {
        return;   
    }
    TArray<FCombatModifier*> RelevantMods;
    CostModifiers.MultiFindPointer(ResourceClass, RelevantMods);
    for (FCombatModifier* Mod : RelevantMods)
    {
        if (Mod && Mod->ID == ModifierID)
        {
            if (CostModifiers.RemoveSingle(ResourceClass, *Mod) != 0)
            {
                RecalculateAbilityCost(ResourceClass);
            }
            break;
        }
    }
}

void UCombatAbility::NotifyOfReplicatedCost(FAbilityCost const& NewCost)
{
    if (IsValid(NewCost.ResourceClass))
    {
        AbilityCosts.Add(NewCost.ResourceClass, NewCost);
    }
}

void UCombatAbility::ActivateCastRestriction(FName const& RestrictionName)
{
    if (RestrictionName.IsValid())
    {
        CustomCastRestrictions.AddUnique(RestrictionName);
    }
    bCustomCastConditionsMet = (CustomCastRestrictions.Num() == 0);
}

void UCombatAbility::DeactivateCastRestriction(FName const& RestrictionName)
{
    if (RestrictionName.IsValid())
    {
        CustomCastRestrictions.RemoveSingleSwap(RestrictionName);
    }
    bCustomCastConditionsMet = (CustomCastRestrictions.Num() == 0);
}

void UCombatAbility::OnRep_OwningComponent()
{
    if (!IsValid(OwningComponent) || bInitialized)
    {
        return;
    }
    OwningComponent->NotifyOfReplicatedAddedAbility(this);
}

void UCombatAbility::OnRep_AbilityCooldown()
{
    PurgeOldPredictions();
    RecalculatePredictedCooldown();
}

void UCombatAbility::OnRep_Deactivated(bool const Previous)
{
    if (bDeactivated && !Previous)
    {
        OnDeactivate();
        OwningComponent->NotifyOfReplicatedRemovedAbility(this);
    }
}

void UCombatAbility::OnInitialize_Implementation()
{
    return;
}

void UCombatAbility::SetupCustomCastRestrictions_Implementation()
{
    return;
}

void UCombatAbility::OnDeactivate_Implementation()
{
    return;
}

void UCombatAbility::OnPredictedTick_Implementation(int32, TArray<FCombatParameter>& PredictionParams)
{
    return;
}

void UCombatAbility::OnServerTick_Implementation(int32, TArray<FCombatParameter>& BroadcastParams)
{
    return;
}

void UCombatAbility::OnServerPredictedTick_Implementation(int32, TArray<FCombatParameter> const& PredictionParams, TArray<FCombatParameter>& BroadcastParams)
{
    return;
}

bool UCombatAbility::OnSimulatedTick_Implementation(int32, TArray<FCombatParameter> const& BroadcastParams)
{
    return false;
}

void UCombatAbility::OnCastComplete_Implementation()
{
    return;
}

void UCombatAbility::OnPredictedCancel_Implementation(TArray<FCombatParameter>& PredictionParams)
{
    return;
}

void UCombatAbility::OnServerPredictedCancel_Implementation(TArray<FCombatParameter> const& PredictionParams, TArray<FCombatParameter>& BroadcastParams)
{
    return;
}

void UCombatAbility::OnServerCancel_Implementation(TArray<FCombatParameter>& BroadcastParams)
{
    return;
}

void UCombatAbility::OnSimulatedCancel_Implementation(TArray<FCombatParameter> const& BroadcastParams)
{
    return;
}

void UCombatAbility::OnCastInterrupted_Implementation(FInterruptEvent const& InterruptEvent)
{
    return;
}

void UCombatAbility::OnAbilityMispredicted_Implementation(int32, FString const& FailReason)
{
    return;
}