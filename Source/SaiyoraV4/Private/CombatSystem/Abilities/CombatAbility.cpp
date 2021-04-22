// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatAbility.h"
#include "AbilityHandler.h"
#include "ResourceHandler.h"
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
    if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return (PredictedCooldown.OnCooldown && PredictedCooldown.CooldownEndTime != 0.0f) ? PredictedCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetWorldTime() : 0.0f;
    }
    return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetWorldTime() : 0.0f;
}

void UCombatAbility::CommitCharges(int32 const PredictionID)
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.CurrentCharges = FMath::Clamp((PreviousCharges - ChargesPerCast), 0, MaxCharges);
    if (PredictionID != 0)
    {
        AbilityCooldown.PredictionID = PredictionID;
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < MaxCharges)
    {
        StartCooldown();
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

float UCombatAbility::GetAbilityCost(FGameplayTag const& ResourceTag) const
{
    if (!ResourceTag.IsValid() || !ResourceTag.MatchesTag(UResourceHandler::GenericResourceTag))
    {
        return -1.0f;
    }
    FAbilityCost const* Cost = AbilityCosts.Find(ResourceTag);
    if (Cost)
    {
        return Cost->Cost;
    }
    return -1.0f;
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

void UCombatAbility::StartCooldown()
{
    FTimerDelegate CooldownDelegate;
    CooldownDelegate.BindUFunction(this, FName(TEXT("CompleteCooldown")));
    float const CooldownLength = GetHandler()->CalculateAbilityCooldown(this);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, CooldownDelegate, CooldownLength, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetWorldTime();
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
        StartCooldown();
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
    for (FAbilityCost const& Cost : DefaultAbilityCosts)
    {
        AbilityCosts.Add(Cost.ResourceTag, Cost);
    }
    SetupCustomCastRestrictions();
    OnInitialize();
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
    OnPredictedTick(TickNumber, PredictionParams);
}

void UCombatAbility::ServerPredictedTick(int32 const TickNumber, FCombatParameters const& PredictionParams,
    FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerPredictedTick(TickNumber, PredictionParams, BroadcastParams);
}

void UCombatAbility::ServerNonPredictedTick(int32 const TickNumber, FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OnServerNonPredictedTick(TickNumber, BroadcastParams);
}

void UCombatAbility::SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        return;
    }
    OnSimulatedTick(TickNumber, BroadcastParams);
}

void UCombatAbility::CompleteCast()
{
    OnCastComplete();
}

void UCombatAbility::InterruptCast(FInterruptEvent const& InterruptEvent)
{
    OnCastInterrupted(InterruptEvent);
}

void UCombatAbility::CancelCast()
{
    OnCastCancelled();
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
