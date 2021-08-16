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
    int32 const CurrentMaxCharges = MaxCharges.GetValue();
    AbilityCooldown.CurrentCharges = FMath::Clamp(PreviousCharges - ChargeCost.GetValue(), 0, CurrentMaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < CurrentMaxCharges)
    {
        StartCooldown();
    }
}

int32 UCombatAbility::AddChargeCostModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !ChargeCost.IsModifiable())
    {
        return -1;
    }
    int32 const PreviousCost = ChargeCost.GetValue();
    int32 const ModID = ChargeCost.AddModifier(Modifier);
    if (PreviousCost != ChargeCost.GetValue())
    {
        CheckChargeCostMet();
    }
    return ModID;
}

void UCombatAbility::RemoveChargeCostModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    int32 const PreviousCost = ChargeCost.GetValue();
    ChargeCost.RemoveModifier(ModifierID);
    if (PreviousCost != ChargeCost.GetValue())
    {
        CheckChargeCostMet();
    }
}

int32 UCombatAbility::AddChargesPerCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !ChargesPerCooldown.IsModifiable())
    {
        return -1;
    }
    return ChargesPerCooldown.AddModifier(Modifier);
}

void UCombatAbility::RemoveChargesPerCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    ChargesPerCooldown.RemoveModifier(ModifierID);
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

float UCombatAbility::GetAbilityCost(TSubclassOf<UResource> const ResourceClass) const
{
    if (IsValid(ResourceClass))
    {
        FAbilityResourceCost const* Cost = AbilityCosts.Find(ResourceClass);
        if (Cost)
        {
            return Cost->Cost.GetValue();
        }
    }
    return 0.0f;
}

void UCombatAbility::GetAbilityCosts(TMap<TSubclassOf<UResource>, float>& OutCosts) const
{
    for (TTuple<TSubclassOf<UResource>, FAbilityResourceCost> const& Cost : AbilityCosts)
    {
        if (IsValid(Cost.Key))
        {
            OutCosts.Add(Cost.Key, Cost.Value.Cost.GetValue());
        }
    }
}

void UCombatAbility::ForceResourceCostRecalculations()
{
    bool CostChanged = false;
    for (TTuple<TSubclassOf<UResource>, FAbilityResourceCost>& Cost : AbilityCosts)
    {
        float const Previous = Cost.Value.Cost.GetValue();
        Cost.Value.Cost.ForceRecalculation();
        if (Previous != Cost.Value.Cost.GetValue())
        {
            CostChanged = true;
        }
    }
    if (CostChanged)
    {
        CheckResourceCostsMet();
    }
}

void UCombatAbility::CheckResourceCostsMet()
{
    bool const PreviouslyMet = ResourceCostsMet;
    if (AbilityCosts.Num() == 0)
    {
        ResourceCostsMet = true;
    }
    else if (!IsValid(OwningComponent) || !IsValid(OwningComponent->GetResourceHandlerRef()))
    {   
        ResourceCostsMet = false;
    }
    else
    {
        TMap<TSubclassOf<UResource>, float> Costs;
        GetAbilityCosts(Costs);
        ResourceCostsMet = OwningComponent->GetResourceHandlerRef()->CheckAbilityCostsMet(Costs);
    }
    if (PreviouslyMet != ResourceCostsMet)
    {
        UpdateCastable();
    }
}

void UCombatAbility::CheckResourceCostsOnResourceChanged(UResource* Resource, UObject* ChangeSource,
    FResourceState const& PreviousState, FResourceState const& NewState)
{
    if (IsValid(Resource))
    {
        float const Cost = GetAbilityCost(Resource->GetClass());
        if (Cost > NewState.CurrentValue)
        {
            CheckResourceCostsMet();
        }
    }
}

float UCombatAbility::RecalculateCastLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
    if (!IsValid(OwningComponent))
    {
        return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
    }
    TArray<FCombatModifier> Mods = SpecificMods;
    OwningComponent->GetCastLengthModifiers(GetClass(), Mods);
    return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void UCombatAbility::ForceCastLengthRecalculation()
{
    CastLength.ForceRecalculation();
}

void UCombatAbility::UpdateCastable()
{
    ECastFailReason const PreviousCastable = Castable;
    if (!bInitialized)
    {
        Castable = ECastFailReason::InvalidAbility;
    }
    else if (!ChargeCostMet)
    {
        Castable = ECastFailReason::ChargesNotMet;
    }
    else if (!ResourceCostsMet)
    {
        Castable = ECastFailReason::CostsNotMet;
    }
    else if (!bCustomCastConditionsMet)
    {
        Castable = ECastFailReason::AbilityConditionsNotMet;
    }
    else
    {
        Castable = ECastFailReason::None;
    }
    if (PreviousCastable != Castable)
    {
        OnCastableChanged.Broadcast(Castable);
    }
}

void UCombatAbility::SubscribeToCastableChanged(FCastableCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnCastableChanged.AddUnique(Callback);
    }
}

void UCombatAbility::UnsubscribeFromCastableChanged(FCastableCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnCastableChanged.Remove(Callback);
    }   
}

int32 UCombatAbility::AddGlobalCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !GlobalCooldownLength.IsModifiable())
    {
        return -1;
    }
    return GlobalCooldownLength.AddModifier(Modifier);
}

void UCombatAbility::RemoveGlobalCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    GlobalCooldownLength.RemoveModifier(ModifierID);
}

float UCombatAbility::RecalculateGcdLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
    if (!IsValid(OwningComponent))
    {
        return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
    }
    TArray<FCombatModifier> Mods = SpecificMods;
    OwningComponent->GetGlobalCooldownModifiers(this->GetClass(), Mods);
    return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void UCombatAbility::ForceGlobalCooldownRecalculation()
{
    GlobalCooldownLength.ForceRecalculation();
}

int32 UCombatAbility::AddCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !CooldownLength.IsModifiable())
    {
        return -1;
    }
    return CooldownLength.AddModifier(Modifier);
}

void UCombatAbility::RemoveCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    CooldownLength.RemoveModifier(ModifierID);
}

int32 UCombatAbility::AddMaxChargeModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !MaxCharges.IsModifiable())
    {
        return -1;
    }
    return MaxCharges.AddModifier(Modifier);
}

void UCombatAbility::RemoveMaxChargeModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    MaxCharges.RemoveModifier(ModifierID);
}

float UCombatAbility::RecalculateCooldownLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
    if (!IsValid(OwningComponent))
    {
        return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
    }
    TArray<FCombatModifier> Mods = SpecificMods;
    OwningComponent->GetCooldownModifiers(GetClass(), Mods);
    return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void UCombatAbility::ForceCooldownLengthRecalculation()
{
    CooldownLength.ForceRecalculation();
}

void UCombatAbility::CheckChargeCostMet()
{
    bool const PreviouslyMet = ChargeCostMet;
    ChargeCostMet = AbilityCooldown.CurrentCharges >= ChargeCost.GetValue();
    if (PreviouslyMet != ChargeCostMet)
    {
        UpdateCastable();
    }
}

int32 UCombatAbility::AddCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
    if (!IsValid(ResourceClass) || Modifier.GetModType() == EModifierType::Invalid)
    {
        return -1;
    }
    FAbilityResourceCost* Cost = AbilityCosts.Find(ResourceClass);
    if (!Cost || !Cost->Cost.IsModifiable())
    {
        return -1;
    }
    float const PreviousCost = Cost->Cost.GetValue();
    int32 const ModID = Cost->Cost.AddModifier(Modifier);
    if (PreviousCost != Cost->Cost.GetValue())
    {
        CheckResourceCostsMet();
    }
    return ModID;
}

void UCombatAbility::RemoveCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID)
{
    if (!IsValid(ResourceClass) || ModifierID == -1)
    {
        return;
    }
    FAbilityResourceCost* Cost = AbilityCosts.Find(ResourceClass);
    if (!Cost || !Cost->Cost.IsModifiable())
    {
        return;
    }
    float const PreviousCost = Cost->Cost.GetValue();
    Cost->Cost.RemoveModifier(ModifierID);
    if (PreviousCost != Cost->Cost.GetValue())
    {
        CheckResourceCostsMet();
    }
}

void UCombatAbility::StartCooldown()
{
    float const Cooldown = CooldownLength.GetValue();
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, Cooldown, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + Cooldown;
}

void UCombatAbility::CompleteCooldown()
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    int32 const CurrentMaxCharges = MaxCharges.GetValue();
    AbilityCooldown.CurrentCharges = FMath::Clamp(GetCurrentCharges() + ChargesPerCooldown.GetValue(), 0, CurrentMaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (AbilityCooldown.CurrentCharges < CurrentMaxCharges)
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
    if (!IsValid(AbilityComponent))
    {
        return;
    }
    OwningComponent = AbilityComponent;
    
    MaxCharges = FCombatIntValue(DefaultMaxCharges, !bStaticMaxCharges, true, 1);
    AbilityCooldown.CurrentCharges = MaxCharges.GetValue();
    ChargesPerCooldown = FCombatIntValue(DefaultChargesPerCooldown, !bStaticChargesPerCooldown, true, 0);
    ChargeCost = FCombatIntValue(DefaultChargeCost, !bStaticChargeCost, true, 0);
    CooldownLength = FCombatFloatValue(DefaultCooldown, !bStaticCooldown, true, UAbilityHandler::MinimumCooldownLength);
    if (!bStaticCooldown)
    {
        FCombatValueRecalculation CooldownRecalculation;
        CooldownRecalculation.BindUObject(this, &UCombatAbility::RecalculateCooldownLength);
        CooldownLength.SetRecalculationFunction(CooldownRecalculation);
        FGenericCallback CooldownModsCallback;
        CooldownModsCallback.BindDynamic(this, &UCombatAbility::ForceCooldownLengthRecalculation);
        OwningComponent->SubscribeToCooldownModsChanged(CooldownModsCallback);
    }
    GlobalCooldownLength = FCombatFloatValue(DefaultGlobalCooldownLength, !bStaticGlobalCooldown, true, UAbilityHandler::MinimumGlobalCooldownLength);
    if (!bStaticGlobalCooldown)
    {
        FCombatValueRecalculation GcdRecalculation;
        GcdRecalculation.BindUObject(this, &UCombatAbility::RecalculateGcdLength);
        GlobalCooldownLength.SetRecalculationFunction(GcdRecalculation);
        FGenericCallback GcdModsCallback;
        GcdModsCallback.BindDynamic(this, &UCombatAbility::ForceGlobalCooldownRecalculation);
        OwningComponent->SubscribeToGlobalCooldownModsChanged(GcdModsCallback);
    }
    if (CastType == EAbilityCastType::Channel)
    {
        CastLength = FCombatFloatValue(DefaultCastTime, !bStaticCastTime, true, UAbilityHandler::MinimumCastLength);
        if (!bStaticCastTime)
        {
            FCombatValueRecalculation CastRecalculation;
            CastRecalculation.BindUObject(this, &UCombatAbility::RecalculateCastLength);
            CastLength.SetRecalculationFunction(CastRecalculation);
            FGenericCallback CastModsCallback;
            CastModsCallback.BindDynamic(this, &UCombatAbility::ForceCastLengthRecalculation);
            OwningComponent->SubscribeToCastModsChanged(CastModsCallback);
        }
    }
    bool bAllStaticCosts = true;
    for (FAbilityCost const& DefaultCost : DefaultAbilityCosts)
    {
        if (IsValid(DefaultCost.ResourceClass))
        {
            FAbilityResourceCost& NewCost = AbilityCosts.Add(DefaultCost.ResourceClass);
            NewCost.Initialize(DefaultCost.ResourceClass, GetClass(), OwningComponent);
            if (!DefaultCost.bStaticCost)
            {
                bAllStaticCosts = false;
            }
        }
    }
    if (!bAllStaticCosts)
    {
        FGenericCallback CostModsCallback;
        CostModsCallback.BindDynamic(this, &UCombatAbility::ForceResourceCostRecalculations);
        OwningComponent->SubscribeToCostModsChanged(CostModsCallback);
    }
    if (IsValid(OwningComponent->GetResourceHandlerRef()))
    {
        for (TTuple<TSubclassOf<UResource>, FAbilityResourceCost> const& Cost : AbilityCosts)
        {
            UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(Cost.Key);
            if (IsValid(Resource))
            {
                FResourceValueCallback OnResourceChanged;
                OnResourceChanged.BindDynamic(this, &UCombatAbility::CheckResourceCostsOnResourceChanged);
                Resource->SubscribeToResourceChanged(OnResourceChanged);
            }
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

int32 UCombatAbility::AddCastLengthModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !CastLength.IsModifiable())
    {
        return -1;
    }
    return CastLength.AddModifier(Modifier);
}

void UCombatAbility::RemoveCastLengthModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    CastLength.RemoveModifier(ModifierID);
}

ECastFailReason UCombatAbility::IsCastable(TMap<TSubclassOf<UResource>, float>& OutCosts) const
{
    GetAbilityCosts(OutCosts);
    return Castable;
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
    int32 const CurrentMaxCharges = MaxCharges.GetValue();
    if (bAdditive)
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + Charges, 0, CurrentMaxCharges);
    }
    else
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(Charges, 0, CurrentMaxCharges);
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        if (AbilityCooldown.CurrentCharges == CurrentMaxCharges && GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
        {
            GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
            AbilityCooldown.OnCooldown = false;
            return;
        }
        if (AbilityCooldown.CurrentCharges < CurrentMaxCharges && !GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
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