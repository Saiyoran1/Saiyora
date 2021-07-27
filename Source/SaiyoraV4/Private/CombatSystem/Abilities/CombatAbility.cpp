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
        return;
    }
    TArray<FCombatModifier> RelevantMods;
    CostModifiers.MultiFind(ResourceClass, RelevantMods);
    MutableCost.Cost = FMath::Max(0.0f, FCombatModifier::CombineModifiers(RelevantMods, MutableCost.Cost));
    AbilityCosts.Add(ResourceClass, MutableCost);
}

void UCombatAbility::StartCooldown()
{
    float const CooldownLength = GetHandler()->CalculateAbilityCooldown(this);
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
    //ReplicatedCosts.Ability = this;
    for (FAbilityCost const& Cost : DefaultAbilityCosts)
    {
        AbilityCosts.Add(Cost.ResourceClass, Cost);
        /*if (AbilityComponent->GetOwnerRole() == ROLE_Authority)
        {
            FReplicatedAbilityCost ReplicatedCost;
            ReplicatedCost.AbilityCost = Cost;
            ReplicatedCosts.MarkItemDirty(ReplicatedCosts.Items.Add_GetRef(ReplicatedCost));
        }*/
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

void UCombatAbility::OnSimulatedCancel_Implementation(TArray<FCombatParameter> const& BroadcastParams)
{
    return;
}

void UCombatAbility::OnCastInterrupted_Implementation(FInterruptEvent const& InterruptEvent)
{
    return;
}