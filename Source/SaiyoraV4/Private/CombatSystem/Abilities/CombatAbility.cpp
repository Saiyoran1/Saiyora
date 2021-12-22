#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

#pragma region Setup

UWorld* UCombatAbility::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UCombatAbility::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatAbility, OwningComponent);
    DOREPLIFETIME(UCombatAbility, bDeactivated);
    DOREPLIFETIME_CONDITION(UCombatAbility, AbilityCooldown, COND_OwnerOnly);
}

void UCombatAbility::PostNetReceive()
{
    if (bInitialized || bDeactivated)
    {
        return;
    }
    if (IsValid(OwningComponent))
    {
        OwningComponent->NotifyOfReplicatedAbility(this);
    }
}

void UCombatAbility::InitializeAbility(UAbilityComponent* AbilityComponent)
{
    if (bInitialized || !IsValid(AbilityComponent) || AbilityComponent->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    OwningComponent = AbilityComponent;
    AbilityCooldown.MaxCharges = FMath::Max(1, DefaultMaxCharges);
    AbilityCooldown.CurrentCharges = AbilityCooldown.MaxCharges;
    ChargesPerCooldown = FMath::Max(0, DefaultChargesPerCooldown);
    ChargeCost = FMath::Max(0, DefaultChargeCost);
    //TODO: Initialize Costs
    
    SetupCustomCastRestrictions();
    PreInitializeAbility();
    bInitialized = true;
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
    UpdateCastable();
}

void UCombatAbility::DeactivateAbility()
{
    PreDeactivateAbility();
    if (GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
    }
    OnChargesChanged.Clear();
    bDeactivated = true;
}

void UCombatAbility::OnRep_Deactivated(bool const Previous)
{
    if (bDeactivated && bInitialized)
    {
        PreDeactivateAbility();
        OwningComponent->NotifyOfReplicatedAbilityRemoval(this);
    }
}

#pragma endregion
#pragma region Cooldown

bool UCombatAbility::IsCooldownActive() const
{
    if (!IsValid(OwningComponent))
    {
        return false;
    }
    switch (OwningComponent->GetOwnerRole())
    {
        case ROLE_Authority :
            return AbilityCooldown.OnCooldown;
        case ROLE_AutonomousProxy :
            return ClientCooldown.OnCooldown;
        default :
            return false;
    }
}

float UCombatAbility::GetRemainingCooldown() const
{
    if (!IsValid(OwningComponent))
    {
        return 0.0f;
    }
    switch (OwningComponent->GetOwnerRole())
    {
        case ROLE_Authority :
            return AbilityCooldown.OnCooldown ? FMath::Max(0.0f, AbilityCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds()) : 0.0f;
        case ROLE_AutonomousProxy :
            return ClientCooldown.OnCooldown && ClientCooldown.CooldownEndTime != 0.0f ? FMath::Max(0.0f, ClientCooldown.CooldownEndTime - OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds()) : 0.0f;
        default :
            return 0.0f;
    }
}

float UCombatAbility::GetCurrentCooldownLength() const
{
    if (!IsValid(OwningComponent))
    {
        return 0.0f;
    }
    switch (OwningComponent->GetOwnerRole())
    {
        case ROLE_Authority :
            return AbilityCooldown.OnCooldown ? FMath::Max(UAbilityComponent::MinimumCooldownLength, AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime) : 0.0f;
        case ROLE_AutonomousProxy :
            return ClientCooldown.OnCooldown && ClientCooldown.CooldownEndTime != 0.0f ? FMath::Max(UAbilityComponent::MinimumCooldownLength, ClientCooldown.CooldownEndTime - ClientCooldown.CooldownStartTime) : 0.0f;
        default :
            return 0.0f;
    }
}

void UCombatAbility::ModifyCurrentCharges(int32 const Charges, EChargeModificationType const ModificationType)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || ModificationType == EChargeModificationType::None)
    {
        return;
    }
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    switch (ModificationType)
    {
    case EChargeModificationType::Additive :
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + Charges, 0, AbilityCooldown.MaxCharges);
        break;
    case EChargeModificationType::Override :
        AbilityCooldown.CurrentCharges = FMath::Clamp(Charges, 0, AbilityCooldown.MaxCharges);
        break;
    default :
        break;
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        UpdateCastable();
        if (AbilityCooldown.CurrentCharges == AbilityCooldown.MaxCharges && AbilityCooldown.OnCooldown)
        {
            CancelCooldown();
        }
        else if (AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges && !AbilityCooldown.OnCooldown)
        {
            StartCooldown();
        }
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
}

void UCombatAbility::CommitCharges(int32 const PredictionID)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        bool bUseLagCompensation = false;
        if (PredictionID != 0)
        {
            AbilityCooldown.PredictionID = PredictionID;
            bUseLagCompensation = true;
        }
        int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - ChargeCost, 0, AbilityCooldown.MaxCharges);
        if (AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges && !AbilityCooldown.OnCooldown)
        {
            StartCooldown(bUseLagCompensation);
        }
        if (PreviousCharges != AbilityCooldown.CurrentCharges)
        {
            OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        }
    }
    else if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        ChargePredictions.Add(PredictionID, ChargeCost);
        RecalculatePredictedCooldown();
    }
}

void UCombatAbility::StartCooldown(bool const bUseLagCompensation)
{
    float const CooldownLength = OwningComponent->CalculateCooldownLength(this, bUseLagCompensation);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, OwningComponent->CalculateCooldownLength(this, CooldownLength), false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + CooldownLength;
}

void UCombatAbility::CompleteCooldown()
{
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    bool const bChargeCostPreviouslyMet = PreviousCharges >= ChargeCost;
    AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + ChargesPerCooldown, 0, AbilityCooldown.MaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        if (!bChargeCostPreviouslyMet && AbilityCooldown.CurrentCharges >= ChargeCost)
        {
            UpdateCastable();
        }
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
    if (AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
    {
        StartCooldown();
    }
    else
    {
        AbilityCooldown.OnCooldown = false;
        AbilityCooldown.CooldownStartTime = 0.0f;
        AbilityCooldown.CooldownEndTime = 0.0f;
    }
}

void UCombatAbility::CancelCooldown()
{
    AbilityCooldown.OnCooldown = false;
    AbilityCooldown.CooldownStartTime = 0.0f;
    AbilityCooldown.CooldownEndTime = 0.0f;
    GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
}

void UCombatAbility::OnRep_AbilityCooldown(FAbilityCooldown const& PreviousState)
{
    if (PreviousState.PredictionID != AbilityCooldown.PredictionID)
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
    RecalculatePredictedCooldown();
}

void UCombatAbility::RecalculatePredictedCooldown()
{
    int32 const PreviousCharges = ClientCooldown.CurrentCharges;
    bool const bChargeCostPreviouslyMet = ClientCooldown.CurrentCharges >= ChargeCost;
    ClientCooldown = AbilityCooldown;
    for (TTuple<int32, int32> const& Prediction : ChargePredictions)
    {
        ClientCooldown.CurrentCharges = FMath::Clamp(ClientCooldown.CurrentCharges - Prediction.Value, 0, AbilityCooldown.MaxCharges);
    }
    //Accept authority cooldown progress if server says we are on cd.
    //If server is not on CD, but predicted charges are less than max, predict a CD has started but do not predict start/end time.
    if (!ClientCooldown.OnCooldown && ClientCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
    {
        ClientCooldown.OnCooldown = true;
        ClientCooldown.CooldownStartTime = 0.0f;
        ClientCooldown.CooldownEndTime = 0.0f;
    }
    if (PreviousCharges != ClientCooldown.CurrentCharges)
    {
        if (bChargeCostPreviouslyMet != (ClientCooldown.CurrentCharges >= ChargeCost))
        {
            UpdateCastable();
        }
        OnChargesChanged.Broadcast(this, PreviousCharges, ClientCooldown.CurrentCharges);
    }
}

#pragma endregion
#pragma region Cost

#pragma endregion
#pragma region Functionality

void UCombatAbility::PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams,
    int32 const PredictionID)
{
    PredictionParameters.ClearParams();
    CurrentPredictionID = PredictionID;
    OnPredictedTick(TickNumber);
    PredictionParams = PredictionParameters;
    CurrentPredictionID = 0;
    PredictionParameters.ClearParams();
}

void UCombatAbility::ServerTick(int32 const TickNumber, FCombatParameters const& PredictionParams,
    FCombatParameters& BroadcastParams, int32 const PredictionID)
{
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
    PredictionParameters = PredictionParams;
    CurrentPredictionID = PredictionID;
    OnServerTick(TickNumber);
    BroadcastParams = BroadcastParameters;
    CurrentPredictionID = 0;
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
}

void UCombatAbility::SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams)
{
    BroadcastParameters.ClearParams();
    BroadcastParameters = BroadcastParams;
    OnSimulatedTick(TickNumber);
    BroadcastParameters.ClearParams();
}

void UCombatAbility::PredictedCancel(FCombatParameters& PredictionParams, int32 const PredictionID)
{
    PredictionParameters.ClearParams();
    CurrentPredictionID = PredictionID;
    OnPredictedCancel();
    PredictionParams = PredictionParameters;
    CurrentPredictionID = PredictionID;
    PredictionParameters.ClearParams();
}

void UCombatAbility::ServerCancel(FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams,
    int32 const PredictionID)
{
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
    PredictionParameters = PredictionParams;
    CurrentPredictionID = PredictionID;
    OnServerCancel();
    BroadcastParams = BroadcastParameters;
    CurrentPredictionID = PredictionID;
    PredictionParameters.ClearParams();
    BroadcastParameters.ClearParams();
}

void UCombatAbility::SimulatedCancel(FCombatParameters const& BroadcastParams)
{
    BroadcastParameters.ClearParams();
    BroadcastParameters = BroadcastParams;
    OnSimulatedCancel();
    BroadcastParameters.ClearParams();
}

void UCombatAbility::ServerInterrupt(FInterruptEvent const& InterruptEvent)
{
    OnServerInterrupt(InterruptEvent);
}

void UCombatAbility::SimulatedInterrupt(FInterruptEvent const& InterruptEvent)
{
    OnSimulatedInterrupt(InterruptEvent);
}

void UCombatAbility::AddPredictionParameter(FCombatParameter const& Parameter)
{
    if (Parameter.ParamType == ECombatParamType::None)
    {
        return;
    }
    PredictionParameters.Parameters.Add(Parameter);
}

FCombatParameter UCombatAbility::GetPredictionParamByName(FString const& Name)
{
    for (FCombatParameter const& Param : PredictionParameters.Parameters)
    {
        if (Param.ParamName == Name)
        {
            return Param;
        }
    }
    return FCombatParameter();
}

FCombatParameter UCombatAbility::GetPredictionParamByType(ECombatParamType const Type)
{
    for (FCombatParameter const& Param : PredictionParameters.Parameters)
    {
        if (Param.ParamType == Type)
        {
            return Param;
        }
    }
    return FCombatParameter();
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

#pragma endregion 
#pragma region Modifiers

void UCombatAbility::AddMaxChargeModifier(UBuff* Source, FCombatModifier const& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticMaxCharges || !IsValid(Source))
    {
        return;
    }
    MaxChargeModifiers.Add(Source, Modifier);
    RecalculateMaxCharges();
}

void UCombatAbility::RemoveMaxChargeModifier(UBuff* Source)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticMaxCharges || !IsValid(Source))
    {
        return;
    }
    if (MaxChargeModifiers.Remove(Source) > 0)
    {
        RecalculateMaxCharges();
    }
}

void UCombatAbility::RecalculateMaxCharges()
{
    int32 const PreviousMaxCharges = AbilityCooldown.MaxCharges;
    int32 const PreviousCharges = AbilityCooldown.CurrentCharges;
    TArray<FCombatModifier> Modifiers;
    MaxChargeModifiers.GenerateValueArray(Modifiers);
    AbilityCooldown.MaxCharges = FCombatModifier::ApplyModifiers(Modifiers, DefaultMaxCharges);
    if (PreviousMaxCharges != AbilityCooldown.MaxCharges)
    {
        if (PreviousCharges == PreviousMaxCharges)
        {
            AbilityCooldown.CurrentCharges = AbilityCooldown.MaxCharges;
        }
        else
        {
            AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges, 0, AbilityCooldown.MaxCharges);
        }
        if (AbilityCooldown.MaxCharges == AbilityCooldown.CurrentCharges && AbilityCooldown.OnCooldown)
        {
            CancelCooldown();
        }
        else if (AbilityCooldown.MaxCharges > AbilityCooldown.CurrentCharges && !AbilityCooldown.OnCooldown)
        {
            StartCooldown();
        }
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        UpdateCastable();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
}

void UCombatAbility::AddChargeCostModifier(UBuff* Source, FCombatModifier const& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargeCost || !IsValid(Source))
    {
        return;
    }
    ChargeCostModifiers.Add(Source, Modifier);
    RecalculateChargeCost();
}

void UCombatAbility::RemoveChargeCostModifier(UBuff* Source)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargeCost || !IsValid(Source))
    {
        return;
    }
    if (ChargeCostModifiers.Remove(Source) > 0)
    {
        RecalculateChargeCost();
    }
}

void UCombatAbility::RecalculateChargeCost()
{
    int32 const PreviousCost = ChargeCost;
    TArray<FCombatModifier> Modifiers;
    ChargeCostModifiers.GenerateValueArray(Modifiers);
    ChargeCost = FCombatModifier::ApplyModifiers(Modifiers, DefaultChargeCost);
    if (ChargeCost != PreviousCost)
    {
        UpdateCastable();
    }
}

void UCombatAbility::AddChargesPerCooldownModifier(UBuff* Source, FCombatModifier const& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargesPerCooldown || !IsValid(Source))
    {
        return;
    }
    ChargesPerCooldownModifiers.Add(Source, Modifier);
    RecalculateChargesPerCooldown();
}

void UCombatAbility::RemoveChargesPerCooldownModifier(UBuff* Source)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargesPerCooldown || !IsValid(Source))
    {
        return;
    }
    if (ChargesPerCooldownModifiers.Remove(Source) > 0)
    {
        RecalculateChargesPerCooldown();
    }
}

void UCombatAbility::RecalculateChargesPerCooldown()
{
    TArray<FCombatModifier> Modifiers;
    ChargesPerCooldownModifiers.GenerateValueArray(Modifiers);
    ChargesPerCooldown = FCombatModifier::ApplyModifiers(Modifiers, DefaultChargesPerCooldown);
}

#pragma endregion
#pragma region Restrictions

void UCombatAbility::UpdateCastable()
{
    if (!bInitialized)
    {
        Castable = ECastFailReason::InvalidAbility;
    }
    else if (GetCurrentCharges() < ChargeCost)
    {
        Castable = ECastFailReason::ChargesNotMet;
    }
    else if (!ResourceCostsMet)
    {
        Castable = ECastFailReason::CostsNotMet;
    }
    else if (!CustomCastRestrictions.Num() > 0)
    {
        Castable = ECastFailReason::AbilityConditionsNotMet;
    }
    else if (RestrictedTags.Num() > 0 || bClassRestricted)
    {
        Castable = ECastFailReason::CustomRestriction;
    }
    else
    {
        Castable = ECastFailReason::None;
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

void UCombatAbility::AddRestrictedTag(FGameplayTag const RestrictedTag)
{
    if (!RestrictedTag.IsValid() || !AbilityTags.HasTag(RestrictedTag))
    {
        return;
    }
    int32 const PreviousRestrictions = RestrictedTags.Num();
    RestrictedTags.Add(RestrictedTag);
    if (PreviousRestrictions == 0 && RestrictedTags.Num() > 0)
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
    int32 const PreviousRestrictions = RestrictedTags.Num();
    RestrictedTags.Remove(RestrictedTag);
    if (PreviousRestrictions > 0 && RestrictedTags.Num() == 0)
    {
        UpdateCastable();
    }
}

void UCombatAbility::ActivateCastRestriction(FGameplayTag const RestrictionTag)
{
    if (!RestrictionTag.IsValid() || !RestrictionTag.MatchesTag(UAbilityComponent::AbilityRestrictionTag()) || RestrictionTag.MatchesTagExact(UAbilityComponent::AbilityRestrictionTag()))
    {
        return;
    }
    int32 const PreviousRestrictions = CustomCastRestrictions.Num();
    CustomCastRestrictions.Add(RestrictionTag);
    if (PreviousRestrictions == 0 && CustomCastRestrictions.Num() > 0)
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
    int32 const PreviousRestrictions = CustomCastRestrictions.Num();
    CustomCastRestrictions.Remove(RestrictionTag);
    if (PreviousRestrictions > 0 && CustomCastRestrictions.Num() == 0)
    {
        UpdateCastable();
    }
}

#pragma endregion 
#pragma region Subscriptions

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

#pragma endregion 

FDefaultAbilityCost UCombatAbility::GetDefaultAbilityCost(TSubclassOf<UResource> const ResourceClass) const
{
    if (IsValid(ResourceClass))
    {
        for (FDefaultAbilityCost const& Cost : DefaultAbilityCosts)
        {
            if (Cost.ResourceClass == ResourceClass)
            {
                return Cost;
            }
        }
    }
    FDefaultAbilityCost const Invalid;
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

void UCombatAbility::CheckChargeCostOnCostUpdated(int32 const Previous, int32 const New)
{
    if (Previous != New)
    {
        CheckChargeCostMet();
    }
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