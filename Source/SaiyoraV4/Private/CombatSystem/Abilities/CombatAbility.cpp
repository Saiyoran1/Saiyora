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
}

UWorld* UCombatAbility::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UCombatAbility::CommitCharges()
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.CurrentCharges = FMath::Clamp((PreviousCharges - ChargesPerCast), 0, MaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < MaxCharges)
    {
        StartCooldown();
    }
}

void UCombatAbility::UpdateChargesPerCast(TArray<FCombatModifier> const& Modifiers)
{
    if (bStaticChargesPerCast)
    {
        return;
    }
    ChargesPerCast = FCombatModifier::ApplyModifiers(Modifiers, DefaultChargesPerCast);
}

void UCombatAbility::UpdateChargesPerCooldown(TArray<FCombatModifier> const& Modifiers)
{
    if (bStaticChargesPerCooldown)
    {
        return;
    }
    ChargesPerCooldown = FCombatModifier::ApplyModifiers(Modifiers, DefaultChargesPerCooldown);
}

FAbilityCost UCombatAbility::GetDefaultAbilityCost(TSubclassOf<UResource> const ResourceClass) const
{
    if (IsValid(ResourceClass))
    {
        for (FAbilityCost const& Cost : DefaultAbilityCosts)
        {
            if (Cost.ResourceClass == ResourceClass)
            {
                return Cost;
            }
        }
    }
    FAbilityCost const Invalid;
    return Invalid;
}

void UCombatAbility::UpdateMaxCharges(TArray<FCombatModifier> const& Modifiers)
{
    if (bStaticMaxCharges)
    {
        return;
    }
    int32 const Previous = MaxCharges;
    MaxCharges = FCombatModifier::ApplyModifiers(Modifiers, DefaultMaxCharges);
    if (Previous != MaxCharges)
    {
        OnMaxChargesChanged.Broadcast(this, Previous, MaxCharges);
    }
    if (GetHandler()->GetOwnerRole() == ROLE_Authority)
    {
        int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges, 0, MaxCharges);
        if (PreviousCharges != AbilityCooldown.CurrentCharges)
        {
            OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        }
        if (AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges == MaxCharges)
        {
            GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
            AbilityCooldown.OnCooldown = false;
        }
        else if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < MaxCharges)
        {
            StartCooldown();
        }
    }
}

void UCombatAbility::SubscribeToMaxChargesChanged(FAbilityChargeCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnMaxChargesChanged.AddUnique(Callback);
}

void UCombatAbility::UnsubscribeFromMaxChargesChanged(FAbilityChargeCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnMaxChargesChanged.Remove(Callback);
}

void UCombatAbility::StartCooldown()
{
    float const CooldownLength = GetHandler()->CalculateCooldownLength(this);
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
        StartCooldown();
    }
    else
    {
        AbilityCooldown.OnCooldown = false;
    }
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

void UCombatAbility::ServerTick(int32 const TickNumber, FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    BroadcastParameters.ClearParams();
    OnServerTick(TickNumber);
    BroadcastParams = BroadcastParameters;
    BroadcastParameters.ClearParams();
}

void UCombatAbility::SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        return;
    }
    BroadcastParameters.ClearParams();
    BroadcastParameters = BroadcastParams;
    OnSimulatedTick(TickNumber);
    BroadcastParameters.ClearParams();
}

void UCombatAbility::CompleteCast()
{
    OnCastComplete();
}

void UCombatAbility::InterruptCast(FInterruptEvent const& InterruptEvent)
{
    OnCastInterrupted(InterruptEvent);
}

void UCombatAbility::ServerCancel(FCombatParameters& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    BroadcastParameters.ClearParams();
    OnServerCancel();
    BroadcastParams = BroadcastParameters;
    BroadcastParameters.ClearParams();
}

void UCombatAbility::SimulatedCancel(FCombatParameters const& BroadcastParams)
{
    if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        return;
    }
    BroadcastParameters.ClearParams();
    BroadcastParameters = BroadcastParams;
    OnSimulatedCancel();
    BroadcastParameters.ClearParams();
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

void UCombatAbility::ModifyCurrentCharges(int32 const Charges, bool const bAdditive)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    if (bAdditive)
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + Charges, 0, MaxCharges);
    }
    else
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(Charges, 0, MaxCharges);
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        if (AbilityCooldown.CurrentCharges == MaxCharges && GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
        {
            GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
            AbilityCooldown.OnCooldown = false;
            return;
        }
        if (AbilityCooldown.CurrentCharges < MaxCharges && !GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
        {
            StartCooldown();
        }
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

void UCombatAbility::OnServerTick_Implementation(int32 const TickNumber)
{
    return;
}

void UCombatAbility::OnSimulatedTick_Implementation(int32)
{
    return;
}

void UCombatAbility::OnCastComplete_Implementation()
{
    return;
}

void UCombatAbility::OnServerCancel_Implementation()
{
    return;
}

void UCombatAbility::OnSimulatedCancel_Implementation()
{
    return;
}

void UCombatAbility::OnCastInterrupted_Implementation(FInterruptEvent const& InterruptEvent)
{
    return;
}