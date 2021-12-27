#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "Resource.h"
#include "ResourceHandler.h"
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
    DOREPLIFETIME_CONDITION(UCombatAbility, bClassRestricted, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, bTagsRestricted, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, ChargeCost, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, AbilityCosts, COND_OwnerOnly);
}

void UCombatAbility::PostNetReceive()
{
    if (bInitialized || bDeactivated)
    {
        return;
    }
    if (IsValid(OwningComponent))
    {
        OnResourceChanged.BindDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
        SetupCustomCastRestrictions();
        UpdateCastable();
        PreInitializeAbility();
        bInitialized = true;
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
    AbilityCosts.OwningAbility = this;
    OnResourceChanged.BindDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
    for (FDefaultAbilityCost const& DefaultCost : DefaultAbilityCosts)
    {
        if (IsValid(DefaultCost.ResourceClass))
        {
            FAbilityCost NewCost;
            NewCost.ResourceClass = DefaultCost.ResourceClass;
            NewCost.Cost = DefaultCost.Cost;
            AbilityCosts.MarkItemDirty(AbilityCosts.Items.Add_GetRef(NewCost));
            bool bCostMet = false;
            if (IsValid(OwningComponent->GetResourceHandlerRef()))
            {
                UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(DefaultCost.ResourceClass);
                if (IsValid(Resource))
                {
                    if (Resource->GetCurrentValue() >= DefaultCost.Cost)
                    {
                         bCostMet = true;
                    }
                    Resource->SubscribeToResourceChanged(OnResourceChanged);
                }
            }
            if (!bCostMet)
            {
                UnmetCosts.Add(DefaultCost.ResourceClass);
            }
        }
    }
    SetupCustomCastRestrictions();
    PreInitializeAbility();
    bInitialized = true;
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

int32 UCombatAbility::GetCurrentCharges() const
{
    switch (OwningComponent->GetOwnerRole())
    {
    case ROLE_Authority :
        return AbilityCooldown.CurrentCharges;
    case ROLE_AutonomousProxy :
        return ClientCooldown.CurrentCharges;
    default :
        return 0;
    }
}

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

float UCombatAbility::GetCooldownLength()
{
    return !bStaticCooldownLength && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateCooldownLength(this, false) : DefaultCooldownLength;
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
            UpdateCastable();
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
    UE_LOG(LogTemp, Warning, TEXT("Cooldown Length was: %f"), CooldownLength);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, CooldownLength, false);
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
        UpdateCastable();
        OnChargesChanged.Broadcast(this, PreviousCharges, ClientCooldown.CurrentCharges);
    }
}

void UCombatAbility::OnRep_ChargeCost()
{
    UpdateCastable();
}

#pragma endregion
#pragma region Cost

void UCombatAbility::UpdateCost(TSubclassOf<UResource> const ResourceClass)
{
    bool bFoundDefault = false;
    float BaseCost = 0.0f;
    bool bStatic = false;
    for (FDefaultAbilityCost const& DefaultCost : DefaultAbilityCosts)
    {
        if (DefaultCost.ResourceClass == ResourceClass)
        {
            bStatic = DefaultCost.bStaticCost;
            bFoundDefault = true;
            BaseCost = DefaultCost.Cost;
        }
    }
    if (!bFoundDefault)
    {
        return;
    }
    TArray<FCombatModifier> CombatModifiers;
    ResourceCostModifiers.MultiFind(ResourceClass, CombatModifiers);
    for (FAbilityCost& Cost : AbilityCosts.Items)
    {
        if (Cost.ResourceClass == ResourceClass)
        {
            Cost.Cost = bStatic ? BaseCost : FCombatModifier::ApplyModifiers(CombatModifiers, BaseCost);
            AbilityCosts.MarkItemDirty(Cost);
            break;
        }
    }
    CheckCostMet(ResourceClass);
}

void UCombatAbility::CheckCostMet(TSubclassOf<UResource> const ResourceClass)
{
    if (!IsValid(ResourceClass) || !IsValid(OwningComponent))
    {
        return;
    }
    bool bCostMet = false;
    for (FAbilityCost const& Cost : AbilityCosts.Items)
    {
        if (Cost.ResourceClass == ResourceClass)
        {
            if (IsValid(OwningComponent->GetResourceHandlerRef()))
            {
                UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(ResourceClass);
                if (IsValid(Resource))
                {
                    if (Resource->GetCurrentValue() >= Cost.Cost)
                    {
                        bCostMet = true;
                    }
                }
            }
            break;
        }
    }
    if (bCostMet)
    {
        if (UnmetCosts.Remove(ResourceClass) > 0 && UnmetCosts.Num() == 0)
        {
            UpdateCastable();
        }
    }
    else
    {
        int32 const PreviousUnmet = UnmetCosts.Num();
        UnmetCosts.Add(ResourceClass);
        if (PreviousUnmet == 0 && UnmetCosts.Num() > 0)
        {
            UpdateCastable();
        }
    }
}

void UCombatAbility::CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource,
    FResourceState const& PreviousState, FResourceState const& NewState)
{
    if (IsValid(Resource))
    {
        bool bCostMet = true;
        for (FAbilityCost const& Cost : AbilityCosts.Items)
        {
            if (Cost.ResourceClass == Resource->GetClass())
            {
                if (NewState.CurrentValue < Cost.Cost)
                {
                    bCostMet = false;
                }
                break;
            }
        }
        if (bCostMet)
        {
            if (UnmetCosts.Remove(Resource->GetClass()) > 0 && UnmetCosts.Num() == 0)
            {
                UpdateCastable();
            }
        }
        else
        {
            int32 const PreviousUnmet = UnmetCosts.Num();
            UnmetCosts.Add(Resource->GetClass());
            if (PreviousUnmet == 0 && UnmetCosts.Num() > 0)
            {
                UpdateCastable();
            }
        }
    }
}

void UCombatAbility::UpdateCostFromReplication(FAbilityCost const& Cost, bool const bNewAdd)
{
    if (IsValid(Cost.ResourceClass) && IsValid(OwningComponent))
    {
        bool bCostMet = false;
        if (IsValid(OwningComponent->GetResourceHandlerRef()))
        {
            UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(Cost.ResourceClass);
            if (IsValid(Resource))
            {
                bCostMet = Resource->GetCurrentValue() >= Cost.Cost;
                if (bNewAdd)
                {
                    Resource->SubscribeToResourceChanged(OnResourceChanged);
                }
            }
        }
        if (bCostMet)
        {
            if (UnmetCosts.Remove(Cost.ResourceClass) > 0 && UnmetCosts.Num() == 0)
            {
                UpdateCastable();
            }
        }
        else
        {
            int32 const PreviousUnmet = UnmetCosts.Num();
            UnmetCosts.Add(Cost.ResourceClass);
            if (PreviousUnmet == 0 && UnmetCosts.Num() > 0)
            {
                UpdateCastable();
            }
        }
    }
}

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

FCombatParameter UCombatAbility::GetPredictionParamByName(FString const& ParamName)
{
    for (FCombatParameter const& Param : PredictionParameters.Parameters)
    {
        if (Param.ParamName == ParamName)
        {
            return Param;
        }
    }
    return FCombatParameter();
}

FCombatParameter UCombatAbility::GetPredictionParamByType(ECombatParamType const ParamType)
{
    for (FCombatParameter const& Param : PredictionParameters.Parameters)
    {
        if (Param.ParamType == ParamType)
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

FCombatParameter UCombatAbility::GetBroadcastParamByName(FString const& ParamName)
{
    for (FCombatParameter const& Param : BroadcastParameters.Parameters)
    {
        if (Param.ParamName == ParamName)
        {
            return Param;
        }
    }
    return FCombatParameter();
}

FCombatParameter UCombatAbility::GetBroadcastParamByType(ECombatParamType const ParamType)
{
    for (FCombatParameter const& Param : BroadcastParameters.Parameters)
    {
        if (Param.ParamType == ParamType)
        {
            return Param;
        }
    }
    return FCombatParameter();
}

void UCombatAbility::UpdatePredictionFromServer(FServerAbilityResult const& Result)
{
    if (AbilityCooldown.PredictionID < Result.PredictionID)
    {
        ChargePredictions.Add(Result.PredictionID, Result.ChargesSpent);
        RecalculatePredictedCooldown();
    }
    if (!Result.bSuccess)
    {
        OnMisprediction(Result.PredictionID, Result.FailReason);
    }
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

void UCombatAbility::AddResourceCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Modifier.Source))
    {
        return;
    }
    TArray<FCombatModifier*> Modifiers;
    ResourceCostModifiers.MultiFindPointer(ResourceClass, Modifiers);
    bool bFound = false;
    for (FCombatModifier* Mod : Modifiers)
    {
        if (Mod->Source == Modifier.Source)
        {
            *Mod = Modifier;
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        ResourceCostModifiers.Add(ResourceClass, Modifier);
    }
    UpdateCost(ResourceClass);
}

void UCombatAbility::RemoveResourceCostModifier(TSubclassOf<UResource> const ResourceClass, UBuff* Source)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Source))
    {
        return;
    }
    TArray<FCombatModifier*> Modifiers;
    ResourceCostModifiers.MultiFindPointer(ResourceClass, Modifiers);
    bool bFound = false;
    for (FCombatModifier* Mod : Modifiers)
    {
        if (Mod->Source == Source)
        {
            ResourceCostModifiers.Remove(ResourceClass, *Mod);
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        return;
    }
    UpdateCost(ResourceClass);
}

float UCombatAbility::GetCastLength()
{
    return !bStaticCastTime && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateCastLength(this, false) : DefaultCastTime;
}

float UCombatAbility::GetGlobalCooldownLength()
{
    return !bStaticGlobalCooldownLength && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateGlobalCooldownLength(this, false) : DefaultGlobalCooldownLength;
}

#pragma endregion
#pragma region Restrictions

void UCombatAbility::UpdateCastable()
{
    if (!bInitialized || bDeactivated)
    {
        Castable = ECastFailReason::InvalidAbility;
    }
    else if (GetCurrentCharges() < ChargeCost)
    {
        Castable = ECastFailReason::ChargesNotMet;
    }
    else if (UnmetCosts.Num() > 0)
    {
        Castable = ECastFailReason::CostsNotMet;
    }
    else if (CustomCastRestrictions.Num() > 0)
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
    RestrictedTags.Add(RestrictedTag);
    if (!bTagsRestricted && RestrictedTags.Num() > 0)
    {
        bTagsRestricted = true;
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
    if (bTagsRestricted && RestrictedTags.Num() == 0)
    {
        bTagsRestricted = false;
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