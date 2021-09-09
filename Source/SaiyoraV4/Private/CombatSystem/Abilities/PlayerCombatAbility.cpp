#include "CombatSystem/Abilities/PlayerCombatAbility.h"
#include "AbilityHandler.h"
#include "PlayerAbilityHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"

void UPlayerCombatAbility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    ResetReplicatedLifetimeProperty(StaticClass(), UCombatAbility::StaticClass(), FName(TEXT("AbilityCooldown")), COND_OwnerOnly, OutLifetimeProps);
}

void UPlayerCombatAbility::PurgeOldPredictions()
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

void UPlayerCombatAbility::RecalculatePredictedCooldown()
{
    FAbilityCooldown const PreviousState = ClientCooldown;
    ClientCooldown =  AbilityCooldown;
    int32 const CurrentMaxCharges = GetMaxCharges();
    for (TTuple<int32, int32> const& Prediction : ChargePredictions)
    {
        ClientCooldown.CurrentCharges = FMath::Clamp(ClientCooldown.CurrentCharges - Prediction.Value, 0, CurrentMaxCharges);
    }
    //We accept authority cooldown progress if server says we are on cd.
    //If server is not on CD, but predicted charges are less than max, we predict a CD has started but do not predict start/end time.
    if (!ClientCooldown.OnCooldown && ClientCooldown.CurrentCharges < CurrentMaxCharges)
    {
        ClientCooldown.OnCooldown = true;
        ClientCooldown.CooldownStartTime = 0.0f;
        ClientCooldown.CooldownEndTime = 0.0f;
    }
    if (PreviousState.CurrentCharges != ClientCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousState.CurrentCharges, ClientCooldown.CurrentCharges);
    }
}

void UPlayerCombatAbility::OnRep_AbilityCooldown()
{
	PurgeOldPredictions();
    RecalculatePredictedCooldown();
}

void UPlayerCombatAbility::AdjustCooldownFromMaxChargesChanged()
{
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        Super::AdjustCooldownFromMaxChargesChanged();
    }
    else if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        RecalculatePredictedCooldown();
    }
}

bool UPlayerCombatAbility::GetCooldownActive() const
{
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        return AbilityCooldown.OnCooldown;
    }
    if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return ClientCooldown.OnCooldown;
    }
    return false;
}

float UPlayerCombatAbility::GetRemainingCooldown() const
{
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        return AbilityCooldown.OnCooldown ?
            FMath::Max(0.0f, AbilityCooldown.CooldownEndTime - GetHandler()->GetGameStateRef()->GetServerWorldTimeSeconds()) : 0.0f;
    }
    if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return ClientCooldown.OnCooldown ?
            ClientCooldown.CooldownEndTime == 0.0f ?
                GetCooldownLength() : FMath::Max(0.0f, ClientCooldown.CooldownEndTime - GetHandler()->GetGameStateRef()->GetServerWorldTimeSeconds()) : 0.0f;
    }
    return 0.0f;
}

float UPlayerCombatAbility::GetCurrentCooldownLength() const
{
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        return AbilityCooldown.OnCooldown ?
            FMath::Max(0.0f, AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime) : 0.0f;
    }
    if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        return ClientCooldown.OnCooldown ?
            ClientCooldown.CooldownEndTime == 0.0f ?
                GetCooldownLength() : FMath::Max(0.0f, ClientCooldown.CooldownEndTime - ClientCooldown.CooldownStartTime) : 0.0f;
    }
    return 0.0f;
}

void UPlayerCombatAbility::CheckChargeCostMet()
{
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        Super::CheckChargeCostMet();
        return;
    }
    if (GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        bool const PreviouslyMet = ChargeCostMet;
        ChargeCostMet = ClientCooldown.CurrentCharges >= GetChargeCost();
        if (PreviouslyMet != ChargeCostMet)
        {
            UpdateCastable();
        }
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
