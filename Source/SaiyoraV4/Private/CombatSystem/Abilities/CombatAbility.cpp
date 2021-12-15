#include "CombatAbility.h"

#include "AbilityComponent.h"
#include "AbilityHandler.h"
#include "AbilityResourceCost.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

void UCombatAbility::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatAbility, OwningComponent);
    DOREPLIFETIME(UCombatAbility, bDeactivated);
    DOREPLIFETIME_CONDITION(UCombatAbility, AbilityCooldown, COND_Never);
}

UWorld* UCombatAbility::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UCombatAbility::CommitCharges(int32 const PredictionID)
{
    //TODO: Charge prediction.
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    int32 const CurrentMaxCharges = GetMaxCharges();
    AbilityCooldown.CurrentCharges = FMath::Clamp(PreviousCharges - GetChargeCost(), 0, CurrentMaxCharges);
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < CurrentMaxCharges)
    {
        StartCooldown();
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        CheckChargeCostMet();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
}

int32 UCombatAbility::AddChargeCostModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !IsValid(ChargeCost) || !ChargeCost->IsModifiable())
    {
        return -1;
    }
    return ChargeCost->AddModifier(Modifier);
}

void UCombatAbility::RemoveChargeCostModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(ChargeCost))
    {
        return;
    }
    ChargeCost->RemoveModifier(ModifierID);
}

int32 UCombatAbility::AddChargesPerCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !IsValid(ChargesPerCooldown) || !ChargesPerCooldown->IsModifiable())
    {
        return -1;
    }
    return ChargesPerCooldown->AddModifier(Modifier);
}

void UCombatAbility::RemoveChargesPerCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(ChargesPerCooldown))
    {
        return;
    }
    ChargesPerCooldown->RemoveModifier(ModifierID);
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
        UAbilityResourceCost* Cost = AbilityCosts.FindRef(ResourceClass);
        if (IsValid(Cost))
        {
            return Cost->GetValue();
        }
    }
    return 0.0f;
}

void UCombatAbility::GetAbilityCosts(TMap<TSubclassOf<UResource>, float>& OutCosts) const
{
    for (TTuple<TSubclassOf<UResource>, UAbilityResourceCost*> const& Cost : AbilityCosts)
    {
        if (IsValid(Cost.Key) && IsValid(Cost.Value))
        {
            OutCosts.Add(Cost.Key, Cost.Value->GetValue());
        }
    }
}

void UCombatAbility::ForceResourceCostRecalculations()
{
    for (TTuple<TSubclassOf<UResource>, UAbilityResourceCost*>& Cost : AbilityCosts)
    {
        if (IsValid(Cost.Value))
        {
            Cost.Value->RecalculateValue();
        }
    }
}

void UCombatAbility::CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource,
    FResourceState const& PreviousState, FResourceState const& NewState)
{
    if (IsValid(Resource))
    {
        if (GetAbilityCost(Resource->GetClass()) > NewState.CurrentValue)
        {
            CostUnmet(Resource->GetClass());
        }
        else
        {
            CostMet(Resource->GetClass());
        }
    }
}

void UCombatAbility::CheckResourceCostOnCostUpdated(TSubclassOf<UResource> ResourceClass, float const NewCost)
{
    if (NewCost == 0.0f)
    {
        CostMet(ResourceClass);
        return;
    }
    if (!IsValid(OwningComponent) || !IsValid(OwningComponent->GetResourceHandlerRef()))
    {   
        CostUnmet(ResourceClass);
        return;
    }
    UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(ResourceClass);
    if (!IsValid(Resource))
    {
        CostUnmet(ResourceClass);
        return;
    }
    if (Resource->GetCurrentValue() <= NewCost)
    {
        CostMet(ResourceClass);
        return;
    }
    CostUnmet(ResourceClass);
}

void UCombatAbility::CostUnmet(TSubclassOf<UResource> ResourceClass)
{
    bool const bPreviouslyMet = ResourceCostsMet;
    UnmetCosts.Add(ResourceClass);
    ResourceCostsMet = UnmetCosts.Num() == 0;
    if (bPreviouslyMet != ResourceCostsMet)
    {
        UpdateCastable();
    }
}

void UCombatAbility::CostMet(TSubclassOf<UResource> ResourceClass)
{
    bool const bPreviouslyMet = ResourceCostsMet;
    UnmetCosts.Remove(ResourceClass);
    ResourceCostsMet = UnmetCosts.Num() == 0;
    if (bPreviouslyMet != ResourceCostsMet)
    {
        UpdateCastable();
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
    if (IsValid(CastLength))
    {
        CastLength->RecalculateValue();
    }
}

void UCombatAbility::AddBroadcastParameter(FCombatParameter const& Parameter)
{
    if (Parameter.ParamType == ECombatParamType::None)
    {
        return;
    }
    BroadcastParameters.Parameters.Add(Parameter);
}

FCombatParameter UCombatAbility::GetBroadcastParamByName(FString const& Name)
{
    for (FCombatParameter const& Param : BroadcastParameters.Parameters)
    {
        if (Param.ParamName == Name)
        {
            return Param;
        }
    }
    return FCombatParameter();
}

FCombatParameter UCombatAbility::GetBroadcastParamByType(ECombatParamType const Type)
{
    for (FCombatParameter const& Param : BroadcastParameters.Parameters)
    {
        if (Param.ParamType == Type)
        {
            return Param;
        }
    }
    return FCombatParameter();
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
    else if (bTagsRestricted || bClassRestricted)
    {
        Castable = ECastFailReason::CustomRestriction;
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

void UCombatAbility::InitialCastableChecks()
{
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
    //TODO: This triggers UpdateCastable multiple times, but I need the ChargeCost check to be a functions so I can override it in the child class.
    //Need a better system.
    CheckChargeCostMet();
    bCustomCastConditionsMet = CustomCastRestrictions.Num() == 0;
    bNoTagsRestricted = RestrictedTags.Num() == 0;
    UpdateCastable();
}

void UCombatAbility::SubscribeToCastableChanged(FCastableCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnCastableChanged.AddUnique(Callback);
}

void UCombatAbility::UnsubscribeFromCastableChanged(FCastableCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnCastableChanged.Remove(Callback);
}

int32 UCombatAbility::AddGlobalCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !IsValid(GlobalCooldownLength) || !GlobalCooldownLength->IsModifiable())
    {
        return -1;
    }
    return GlobalCooldownLength->AddModifier(Modifier);
}

void UCombatAbility::RemoveGlobalCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(GlobalCooldownLength))
    {
        return;
    }
    GlobalCooldownLength->RemoveModifier(ModifierID);
}

float UCombatAbility::RecalculateGcdLength(TArray<FCombatModifier> const& SpecificMods, float const BaseValue)
{
    if (!IsValid(OwningComponent))
    {
        return FCombatModifier::ApplyModifiers(SpecificMods, BaseValue);
    }
    TArray<FCombatModifier> Mods = SpecificMods;
    OwningComponent->GetGlobalCooldownModifiers(GetClass(), Mods);
    return FCombatModifier::ApplyModifiers(Mods, BaseValue);
}

void UCombatAbility::ForceGlobalCooldownRecalculation()
{
    if (IsValid(GlobalCooldownLength))
    {
        GlobalCooldownLength->RecalculateValue();
    }
}

int32 UCombatAbility::AddCooldownModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !IsValid(CooldownLength) || !CooldownLength->IsModifiable())
    {
        return -1;
    }
    return CooldownLength->AddModifier(Modifier);
}

void UCombatAbility::RemoveCooldownModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(CooldownLength))
    {
        return;
    }
    CooldownLength->RemoveModifier(ModifierID);
}

int32 UCombatAbility::AddMaxChargeModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid || !IsValid(MaxCharges) || !MaxCharges->IsModifiable())
    {
        return -1;
    }
    int32 const Previous = MaxCharges->GetValue();
    int32 const ModID = MaxCharges->AddModifier(Modifier);
    if (Previous != MaxCharges->GetValue())
    {
        AdjustCooldownFromMaxChargesChanged();
    }
    return ModID;
}

void UCombatAbility::RemoveMaxChargeModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(MaxCharges))
    {
        return;
    }
    int32 const Previous = MaxCharges->GetValue();
    MaxCharges->RemoveModifier(ModifierID);
    if (Previous != MaxCharges->GetValue())
    {
        AdjustCooldownFromMaxChargesChanged();
    }
}

void UCombatAbility::AdjustCooldownFromMaxChargesChanged()
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        if (MaxCharges->GetValue() > AbilityCooldown.CurrentCharges && !AbilityCooldown.OnCooldown)
        {
            StartCooldown();
        }
        else if (MaxCharges->GetValue() <= AbilityCooldown.CurrentCharges && AbilityCooldown.OnCooldown)
        {
            AbilityCooldown.CurrentCharges = MaxCharges->GetValue();
            CancelCooldown();
        }
    }
}

void UCombatAbility::CancelCooldown()
{
    AbilityCooldown.OnCooldown = false;
    GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
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
    if (IsValid(CooldownLength))
    {
        CooldownLength->RecalculateValue();
    }
}

void UCombatAbility::CheckChargeCostMet()
{
    bool const PreviouslyMet = ChargeCostMet;
    ChargeCostMet = AbilityCooldown.CurrentCharges >= GetChargeCost();
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
    UAbilityResourceCost* Cost = AbilityCosts.FindRef(ResourceClass);
    if (!Cost || !IsValid(Cost) || !Cost->IsModifiable())
    {
        return -1;
    }
    return Cost->AddModifier(Modifier);
}

void UCombatAbility::RemoveCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID)
{
    if (!IsValid(ResourceClass) || ModifierID == -1)
    {
        return;
    }
    UAbilityResourceCost* Cost = AbilityCosts.FindRef(ResourceClass);
    if (!Cost || !IsValid(Cost) || !Cost->IsModifiable())
    {
        return;
    }
    Cost->RemoveModifier(ModifierID);
}

void UCombatAbility::StartCooldown()
{
    float const Cooldown = GetCooldownLength();
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, Cooldown, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + Cooldown;
}

void UCombatAbility::CompleteCooldown()
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    int32 const CurrentMaxCharges = GetMaxCharges();
    AbilityCooldown.CurrentCharges = FMath::Clamp(GetCurrentCharges() + GetChargesPerCooldown(), 0, CurrentMaxCharges);
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

void UCombatAbility::CheckChargeCostOnCostUpdated(int32 const Previous, int32 const New)
{
    if (Previous != New)
    {
        CheckChargeCostMet();
    }
}

void UCombatAbility::InitializeAbility(UAbilityComponent* AbilityComponent)
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
    bInitialized = true;
    if (OwningComponent->GetOwnerRole() != ROLE_SimulatedProxy)
    {
        MaxCharges = NewObject<UModifiableIntValue>(OwningComponent->GetOwner());
        if (IsValid(MaxCharges))
        {
            MaxCharges->Init(DefaultMaxCharges, !bStaticMaxCharges, true, 1);
        }
        AbilityCooldown.CurrentCharges = GetMaxCharges();
    
        ChargesPerCooldown = NewObject<UModifiableIntValue>(OwningComponent->GetOwner());
        if (IsValid(ChargesPerCooldown))
        {
            ChargesPerCooldown->Init(DefaultChargesPerCooldown, !bStaticChargesPerCooldown, true, 0);
        }
    
        ChargeCost = NewObject<UModifiableIntValue>(OwningComponent->GetOwner());
        if (IsValid(ChargeCost))
        {
            ChargeCost->Init(DefaultChargeCost, !bStaticChargeCost, true, 0);
        }
        ChargeCostCallback.BindDynamic(this, &UCombatAbility::CheckChargeCostOnCostUpdated);
        ChargeCost->SubscribeToValueChanged(ChargeCostCallback);
    
        CooldownLength = NewObject<UModifiableFloatValue>(OwningComponent->GetOwner());
        if (IsValid(CooldownLength))
        {
            CooldownLength->Init(DefaultCooldown, !bStaticCooldownLength, true, UAbilityHandler::MinimumCooldownLength);
            if (!bStaticCooldownLength)
            {
                CooldownRecalculation.BindDynamic(this, &UCombatAbility::RecalculateCooldownLength);
                CooldownLength->SetRecalculationFunction(CooldownRecalculation);
                CooldownModsCallback.BindDynamic(this, &UCombatAbility::ForceCooldownLengthRecalculation);
                OwningComponent->SubscribeToCooldownModsChanged(CooldownModsCallback);
            }
        }

        GlobalCooldownLength = NewObject<UModifiableFloatValue>(OwningComponent->GetOwner());
        if (IsValid(GlobalCooldownLength))
        {
            GlobalCooldownLength->Init(DefaultGlobalCooldownLength, !bStaticGlobalCooldownLength, true, UAbilityHandler::MinimumGlobalCooldownLength);
            if (!bStaticGlobalCooldownLength)
            {
                GcdRecalculation.BindDynamic(this, &UCombatAbility::RecalculateGcdLength);
                GlobalCooldownLength->SetRecalculationFunction(GcdRecalculation);
                GcdModsCallback.BindDynamic(this, &UCombatAbility::ForceGlobalCooldownRecalculation);
                OwningComponent->SubscribeToGlobalCooldownModsChanged(GcdModsCallback);
            }
        }
    
        if (CastType == EAbilityCastType::Channel)
        {
            CastLength = NewObject<UModifiableFloatValue>(OwningComponent->GetOwner());
            if (IsValid(CastLength))
            {
                CastLength->Init(DefaultCastTime, !bStaticCastTime, true, UAbilityHandler::MinimumCastLength);
                if (!bStaticCastTime)
                {
                    CastLengthRecalculation.BindDynamic(this, &UCombatAbility::RecalculateCastLength);
                    CastLength->SetRecalculationFunction(CastLengthRecalculation);
                    CastModsCallback.BindDynamic(this, &UCombatAbility::ForceCastLengthRecalculation);
                    OwningComponent->SubscribeToCastModsChanged(CastModsCallback);
                }
            }
        }

        bool bAllStaticCosts = true;
        for (FAbilityCost const& DefaultCost : DefaultAbilityCosts)
        {
            if (IsValid(DefaultCost.ResourceClass))
            {
                UAbilityResourceCost* NewCost = AbilityCosts.Add(DefaultCost.ResourceClass, NewObject<UAbilityResourceCost>(OwningComponent->GetOwner()));
                if (IsValid(NewCost))
                {
                    NewCost->Init(DefaultCost, GetClass(), OwningComponent);
                    if (!DefaultCost.bStaticCost)
                    {
                        bAllStaticCosts = false;
                        FResourceCostCallback CostCallback;
                        CostCallback.BindDynamic(this, &UCombatAbility::CheckResourceCostOnCostUpdated);
                        NewCost->SubscribeToCostChanged(CostCallback);
                    }
                }
            }
        }
        if (!bAllStaticCosts)
        {
            CostModsCallback.BindDynamic(this, &UCombatAbility::ForceResourceCostRecalculations);
            OwningComponent->SubscribeToCostModsChanged(CostModsCallback);
        }
        if (IsValid(OwningComponent->GetResourceHandlerRef()))
        {
            for (TTuple<TSubclassOf<UResource>, UAbilityResourceCost*> const& Cost : AbilityCosts)
            {
                UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(Cost.Key);
                if (IsValid(Resource))
                {
                    FResourceValueCallback OnResourceChanged;
                    OnResourceChanged.BindDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
                    Resource->SubscribeToResourceChanged(OnResourceChanged);
                }
            }
        }
    
        SetupCustomCastRestrictions();
        InitialCastableChecks();
    }
    
    OnInitialize();
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
    if (Modifier.GetModType() == EModifierType::Invalid || CastType != EAbilityCastType::Channel || !IsValid(CastLength) || !CastLength->IsModifiable())
    {
        return -1;
    }
    return CastLength->AddModifier(Modifier);
}

void UCombatAbility::RemoveCastLengthModifier(int32 const ModifierID)
{
    if (ModifierID == -1 || !IsValid(CastLength))
    {
        return;
    }
    CastLength->RemoveModifier(ModifierID);
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

void UCombatAbility::ServerInterrupt(FInterruptEvent const& InterruptEvent)
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

void UCombatAbility::AddRestrictedTag(FGameplayTag const RestrictedTag)
{
    if (!RestrictedTag.IsValid() || !AbilityTags.HasTag(RestrictedTag))
    {
        return;
    }
    RestrictedTags.Add(RestrictedTag);
    bool const bPreviouslyRestricted = bTagsRestricted;
    bTagsRestricted = RestrictedTags.Num() > 0;
    if (bTagsRestricted != bPreviouslyRestricted)
    {
        UpdateCastable();
    }
}

void UCombatAbility::RemoveRestrictedTag(FGameplayTag const RestrictedTag)
{
    if (!RestrictedTag.IsValid() || !AbilityTags.HasTag(RestrictedTag))
    {
        return;
    }
    RestrictedTags.Remove(RestrictedTag);
    bool const bPreviouslyRestricted = bTagsRestricted;
    bTagsRestricted = RestrictedTags.Num() > 0;
    if (bPreviouslyRestricted != bTagsRestricted)
    {
        UpdateCastable();
    }
}

void UCombatAbility::AddClassRestriction()
{
    if (!bClassRestricted)
    {
        bClassRestricted = true;
        UpdateCastable();
    }
}

void UCombatAbility::RemoveClassRestriction()
{
    if (bClassRestricted)
    {
        bClassRestricted = false;
        UpdateCastable();
    }
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
    int32 const CurrentMaxCharges = GetMaxCharges();
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

void UCombatAbility::ActivateCastRestriction(FGameplayTag const RestrictionTag)
{
    if (!RestrictionTag.IsValid() || !RestrictionTag.MatchesTag(UAbilityComponent::AbilityRestrictionTag()) || RestrictionTag.MatchesTagExact(UAbilityComponent::AbilityRestrictionTag()))
    {
        return;
    }
    CustomCastRestrictions.Add(RestrictionTag);
    bool const bPreviouslyMet = bCustomCastConditionsMet;
    bCustomCastConditionsMet = CustomCastRestrictions.Num() == 0;
    if (bPreviouslyMet != bCustomCastConditionsMet)
    {
        UpdateCastable();
    }
}

void UCombatAbility::DeactivateCastRestriction(FGameplayTag const RestrictionTag)
{
    if (!RestrictionTag.IsValid() || !RestrictionTag.MatchesTag(UAbilityComponent::AbilityRestrictionTag()) || RestrictionTag.MatchesTagExact(UAbilityComponent::AbilityRestrictionTag()))
    {
        return;
    }
    CustomCastRestrictions.Remove(RestrictionTag);
    bool const bPreviouslyMet = bCustomCastConditionsMet;
    bCustomCastConditionsMet = CustomCastRestrictions.Num() == 0;
    if (bPreviouslyMet != bCustomCastConditionsMet)
    {
        UpdateCastable();
    }
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