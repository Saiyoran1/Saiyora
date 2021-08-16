#include "CombatSystem/Abilities/PlayerCombatAbility.h"
#include "AbilityHandler.h"
#include "PlayerAbilityHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"

void UPlayerCombatAbility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UPlayerCombatAbility, ReplicatedCooldown, COND_OwnerOnly);
}

void UPlayerCombatAbility::InitializeAbility(UAbilityHandler* AbilityComponent)
{
    Super::InitializeAbility(AbilityComponent);
    if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        //TODO: Setup UI callbacks?
    }
}

void UPlayerCombatAbility::PurgeOldPredictions()
{
	TArray<int32> OldPredictionIDs;
    for (TTuple <int32, int32> const& Prediction : ChargePredictions)
    {
        if (Prediction.Key <= ReplicatedCooldown.PredictionID)
        {
            OldPredictionIDs.AddUnique(Prediction.Key);
        }
    }
    for (int32 const ID : OldPredictionIDs)
    {
        ChargePredictions.Remove(ID);
    }
}

void UPlayerCombatAbility::RecalculatePredictedCooldown()
{
    FAbilityCooldown const PreviousState = AbilityCooldown;
    AbilityCooldown = ReplicatedCooldown;
    int32 const CurrentMaxCharges = GetMaxCharges();
    for (TTuple<int32, int32> const& Prediction : ChargePredictions)
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - Prediction.Value, 0, CurrentMaxCharges);
    }
    //We accept authority cooldown progress if server says we are on cd.
    //If server is not on CD, but predicted charges are less than max, we predict a CD has started but do not predict start/end time.
    if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < CurrentMaxCharges)
    {
        AbilityCooldown.OnCooldown = true;
        AbilityCooldown.CooldownStartTime = 0.0f;
        AbilityCooldown.CooldownEndTime = 0.0f;
    }
    if (PreviousState.CurrentCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousState.CurrentCharges, AbilityCooldown.CurrentCharges);
    }
}

void UPlayerCombatAbility::OnRep_ReplicatedCooldown()
{
	PurgeOldPredictions();
    RecalculatePredictedCooldown();
}

void UPlayerCombatAbility::StartClientCooldownOnMaxChargesChanged(UCombatAbility* Ability, int32 const OldCharges,
    int32 const NewCharges)
{
    //TODO: This function probably doesn't get called anymore?
    if (AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges >= NewCharges)
    {
        AbilityCooldown.OnCooldown = false;
        AbilityCooldown.CooldownStartTime = 0.0f;
        AbilityCooldown.CooldownEndTime = 0.0f;
    }
    else if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < NewCharges)
    {
        AbilityCooldown.OnCooldown = true;
        AbilityCooldown.CooldownStartTime = 0.0f;
        AbilityCooldown.CooldownEndTime = 0.0f;
    }
}

void UPlayerCombatAbility::PredictCommitCharges(int32 const PredictionID)
{
    ChargePredictions.Add(PredictionID, GetChargeCost());
    RecalculatePredictedCooldown();
}

void UPlayerCombatAbility::CommitCharges(int32 const PredictionID)
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    int32 const CurrentMaxCharges = GetMaxCharges();
    AbilityCooldown.CurrentCharges = FMath::Clamp(PreviousCharges - GetChargeCost(), 0, CurrentMaxCharges);
    bool bFromPrediction = false;
    if (PredictionID != 0)
    {
        AbilityCooldown.PredictionID = PredictionID;
        bFromPrediction = true;
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < CurrentMaxCharges)
    {
        if (bFromPrediction)
        {
            StartCooldownFromPrediction();
        }
        else
        {
            StartCooldown();
        }
    }
}

void UPlayerCombatAbility::UpdatePredictedChargesFromServer(int32 const PredictionID, int32 const ChargesSpent)
{
    if (ReplicatedCooldown.PredictionID >= PredictionID)
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

void UPlayerCombatAbility::StartCooldownFromPrediction()
{
    float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetHandler()->GetOwner()), 0.0f, UPlayerAbilityHandler::MaxPingCompensation);
    float const Cooldown = FMath::Max(UAbilityHandler::MinimumCooldownLength, GetCooldownLength() - PingCompensation);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UPlayerCombatAbility::CompleteCooldown, Cooldown, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = GetHandler()->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + Cooldown;
}

void UPlayerCombatAbility::PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams)
{
    PredictionParameters.ClearParams();
    OnPredictedTick(TickNumber);
    PredictionParams = PredictionParameters;
    PredictionParameters.ClearParams();
}

void UPlayerCombatAbility::OnPredictedTick_Implementation(int32)
{
    return;
}

void UPlayerCombatAbility::ServerTick(int32 const TickNumber, FCombatParameters const& PredictionParams,
    FCombatParameters& BroadcastParams)
{
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
    PredictionParameters = PredictionParams;
    OnServerTick(TickNumber);
    BroadcastParams = BroadcastParameters;
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
}

void UPlayerCombatAbility::PredictedCancel(FCombatParameters& PredictionParams)
{
    PredictionParameters.ClearParams();
    OnPredictedCancel();
    PredictionParams = PredictionParameters;
    PredictionParameters.ClearParams();
}

void UPlayerCombatAbility::OnPredictedCancel_Implementation()
{
    return;
}

void UPlayerCombatAbility::ServerCancel(FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams)
{
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
    PredictionParameters = PredictionParams;
    OnServerCancel();
    BroadcastParams = BroadcastParameters;
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
}

void UPlayerCombatAbility::AbilityMisprediction(int32 const PredictionID, ECastFailReason const FailReason)
{
    OnAbilityMispredicted(PredictionID, FailReason);
}

void UPlayerCombatAbility::OnAbilityMispredicted_Implementation(int32, ECastFailReason)
{
    return;
}
