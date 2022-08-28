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
        AbilityCosts.OwningAbility = this;
        for (const FAbilityCost& Cost : AbilityCosts.Items)
        {
            UpdateCostFromReplication(Cost, true);
        }
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
    for (const FDefaultAbilityCost& DefaultCost : DefaultAbilityCosts)
    {
        if (DefaultCost.ResourceClass)
        {
            FAbilityCost NewCost;
            NewCost.ResourceClass = DefaultCost.ResourceClass;
            NewCost.Cost = DefaultCost.Cost;
            AbilityCosts.MarkItemDirty(AbilityCosts.Items.Add_GetRef(NewCost));
            bool bCostMet = false;
            if (OwningComponent->GetResourceHandlerRef())
            {
                UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(DefaultCost.ResourceClass);
                if (IsValid(Resource))
                {
                    if (Resource->GetCurrentValue() >= DefaultCost.Cost)
                    {
                         bCostMet = true;
                    }
                    Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
                }
                else
                {
                    UninitializedResources.Add(DefaultCost.ResourceClass);
                }
            }
            if (!bCostMet)
            {
                UnmetCosts.Add(DefaultCost.ResourceClass);
            }
        }
    }
    if (UninitializedResources.Num() > 0)
    {
        OwningComponent->GetResourceHandlerRef()->OnResourceAdded.AddDynamic(this, &UCombatAbility::SetupCostCheckingForNewResource);
    }
    SetupCustomCastRestrictions();
    PreInitializeAbility();
    bInitialized = true;
    UpdateCastable();
}

void UCombatAbility::SetupCostCheckingForNewResource(UResource* Resource)
{
    if (UninitializedResources.Contains(Resource->GetClass()))
    {
        CheckResourceCostOnResourceChanged(Resource, nullptr, FResourceState(),
            FResourceState(Resource->GetMinimum(), Resource->GetMaximum(), Resource->GetCurrentValue()));
        Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
        UninitializedResources.Remove(Resource->GetClass());
        if (UninitializedResources.Num() <= 0)
        {
            OwningComponent->GetResourceHandlerRef()->OnResourceAdded.RemoveDynamic(this, &UCombatAbility::SetupCostCheckingForNewResource);
        }
    }
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

void UCombatAbility::OnRep_Deactivated()
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
        OwningComponent->CalculateCooldownLength(this) : DefaultCooldownLength;
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
            return AbilityCooldown.OnCooldown ? FMath::Max(UAbilityComponent::MINCDLENGTH, AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime) : 0.0f;
        case ROLE_AutonomousProxy :
            return ClientCooldown.OnCooldown && ClientCooldown.CooldownEndTime != 0.0f ? FMath::Max(UAbilityComponent::MINCDLENGTH, ClientCooldown.CooldownEndTime - ClientCooldown.CooldownStartTime) : 0.0f;
        default :
            return 0.0f;
    }
}

void UCombatAbility::ModifyCurrentCharges(const int32 Charges, const EChargeModificationType ModificationType)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || ModificationType == EChargeModificationType::None)
    {
        return;
    }
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
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

void UCombatAbility::CommitCharges(const int32 PredictionID)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        if (PredictionID != 0)
        {
            AbilityCooldown.PredictionID = PredictionID;
        }
        const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - ChargeCost, 0, AbilityCooldown.MaxCharges);
        if (AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges && !AbilityCooldown.OnCooldown)
        {
            StartCooldown();
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

void UCombatAbility::StartCooldown()
{
    const float CooldownLength = OwningComponent->CalculateCooldownLength(this);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, CooldownLength, false);
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        AbilityCooldown.OnCooldown = true;
        AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
        AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + CooldownLength;
    }
    else if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        ClientCooldown.OnCooldown = true;
        ClientCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
        ClientCooldown.CooldownEndTime = ClientCooldown.CooldownStartTime + CooldownLength;
    }
}

void UCombatAbility::CompleteCooldown()
{
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
    const bool bChargeCostPreviouslyMet = PreviousCharges >= ChargeCost;
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
    const int32 PreviousCharges = ClientCooldown.CurrentCharges;
    ClientCooldown = AbilityCooldown;
    for (const TTuple<int32, int32>& Prediction : ChargePredictions)
    {
        ClientCooldown.CurrentCharges = FMath::Clamp(ClientCooldown.CurrentCharges - Prediction.Value, 0, AbilityCooldown.MaxCharges);
    }
    //Accept authority cooldown progress if server says we are on cd.
    //If server is not on CD, but predicted charges are less than max, predict a CD has started but do not predict start/end time.
    if (!ClientCooldown.OnCooldown && ClientCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
    {
        //TODO: Prediction of cooldown length, based on time since the charges were spent.
        //We need to predict at the time when the charges are spent, so may need a more detailed history.
        StartCooldown();
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

void UCombatAbility::UpdateCost(const TSubclassOf<UResource> ResourceClass)
{
    bool bFoundDefault = false;
    float BaseCost = 0.0f;
    bool bStatic = false;
    for (const FDefaultAbilityCost& DefaultCost : DefaultAbilityCosts)
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

void UCombatAbility::CheckCostMet(const TSubclassOf<UResource> ResourceClass)
{
    if (!IsValid(ResourceClass) || !IsValid(OwningComponent))
    {
        return;
    }
    bool bCostMet = false;
    for (const FAbilityCost& Cost : AbilityCosts.Items)
    {
        if (Cost.ResourceClass == ResourceClass)
        {
            if (IsValid(OwningComponent->GetResourceHandlerRef()))
            {
                const UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(ResourceClass);
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
        const int32 PreviousUnmet = UnmetCosts.Num();
        UnmetCosts.Add(ResourceClass);
        if (PreviousUnmet == 0 && UnmetCosts.Num() > 0)
        {
            UpdateCastable();
        }
    }
}

void UCombatAbility::CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource,
    const FResourceState& PreviousState, const FResourceState& NewState)
{
    if (IsValid(Resource))
    {
        bool bCostMet = false;
        for (const FAbilityCost& Cost : AbilityCosts.Items)
        {
            if (Cost.ResourceClass == Resource->GetClass())
            {
                if (NewState.CurrentValue >= Cost.Cost)
                {
                    bCostMet = true;
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
            const int32 PreviousUnmet = UnmetCosts.Num();
            UnmetCosts.Add(Resource->GetClass());
            if (PreviousUnmet == 0 && UnmetCosts.Num() > 0)
            {
                UpdateCastable();
            }
        }
    }
}

void UCombatAbility::UpdateCostFromReplication(const FAbilityCost& Cost, const bool bNewAdd)
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
                    Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::CheckResourceCostOnResourceChanged);
                }
            }
            else if (bNewAdd)
            {
                UninitializedResources.Add(Cost.ResourceClass);
                if (UninitializedResources.Num() == 1)
                {
                    OwningComponent->GetResourceHandlerRef()->OnResourceAdded.AddDynamic(this, &UCombatAbility::SetupCostCheckingForNewResource);
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
            const int32 PreviousUnmet = UnmetCosts.Num();
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

void UCombatAbility::PredictedTick(const int32 TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID)
{
    CurrentPredictionID = PredictionID;
    CurrentTick = TickNumber;
    CurrentTargets.Empty();
    AbilityOrigin = Origin;
    OnPredictedTick(TickNumber);
    Origin = AbilityOrigin;
    AbilityOrigin.Clear();
    Targets = CurrentTargets;
    CurrentTargets.Empty();
    CurrentTick = 0;
    CurrentPredictionID = 0;
}

void UCombatAbility::ServerTick(const int32 TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID)
{
    CurrentPredictionID = PredictionID;
    CurrentTick = TickNumber;
    CurrentTargets = Targets;
    AbilityOrigin = Origin;
    OnServerTick(TickNumber);
    Origin = AbilityOrigin;
    AbilityOrigin.Clear();
    Targets = CurrentTargets;
    CurrentTargets.Empty();
    CurrentTick = 0;
    CurrentPredictionID = 0;
}

void UCombatAbility::SimulatedTick(const int32 TickNumber, const FAbilityOrigin& Origin, const TArray<FAbilityTargetSet>& Targets)
{
    CurrentTick = TickNumber;
    CurrentTargets = Targets;
    AbilityOrigin = Origin;
    OnSimulatedTick(TickNumber);
    AbilityOrigin.Clear();
    CurrentTargets.Empty();
    CurrentTick = 0;
}

void UCombatAbility::PredictedCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID)
{
    CurrentPredictionID = PredictionID;
    CurrentTargets.Empty();
    AbilityOrigin = Origin;
    OnPredictedCancel();
    Origin = AbilityOrigin;
    AbilityOrigin.Clear();
    Targets = CurrentTargets;
    CurrentTargets.Empty();
    CurrentPredictionID = 0;
}

void UCombatAbility::ServerCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID)
{
    CurrentPredictionID = PredictionID;
    CurrentTargets = Targets;
    AbilityOrigin = Origin;
    OnServerCancel();
    Origin = AbilityOrigin;
    AbilityOrigin.Clear();
    Targets = CurrentTargets;
    CurrentTargets.Empty();
    CurrentPredictionID = 0;
}

void UCombatAbility::SimulatedCancel(const FAbilityOrigin& Origin, const TArray<FAbilityTargetSet>& Targets)
{
    CurrentTargets = Targets;
    AbilityOrigin = Origin;
    OnSimulatedCancel();
    AbilityOrigin.Clear();
    CurrentTargets.Empty();
}

void UCombatAbility::ServerInterrupt(const FInterruptEvent& InterruptEvent)
{
    OnServerInterrupt(InterruptEvent);
}

void UCombatAbility::SimulatedInterrupt(const FInterruptEvent& InterruptEvent)
{
    OnSimulatedInterrupt(InterruptEvent);
}

void UCombatAbility::UpdatePredictionFromServer(const FServerAbilityResult& Result)
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

void UCombatAbility::AddTarget(AActor* Target, const int32 SetID)
{
    for (FAbilityTargetSet& Set : CurrentTargets)
    {
        if (Set.SetID == SetID)
        {
            Set.Targets.Add(Target);
            return;
        }
    }
}

void UCombatAbility::RemoveTarget(AActor* Target)
{
    if (!IsValid(Target))
    {
        return;
    }
    for (FAbilityTargetSet& Set : CurrentTargets)
    {
        for (int i = 0; i < Set.Targets.Num(); i++)
        {
            if (Set.Targets[i] == Target)
            {
                Set.Targets.RemoveAt(i);
                return;
            }
        }
    }
}

void UCombatAbility::RemoveTargetFromAllSets(AActor* Target)
{
    if (!IsValid(Target))
    {
        return;
    }
    for (FAbilityTargetSet& Set : CurrentTargets)
    {
        for (int i = Set.Targets.Num() - 1; i >= 0; i--)
        {
            if (Set.Targets[i] == Target)
            {
                Set.Targets.RemoveAt(i);
            }
        }
    }
}

void UCombatAbility::RemoveTargetSet(const int32 SetID)
{
    for (int i = 0; i < CurrentTargets.Num(); i++)
    {
        if (CurrentTargets[i].SetID == SetID)
        {
            CurrentTargets.RemoveAt(i);
            return;
        }
    }
}

void UCombatAbility::RemoveTargetFromSet(AActor* Target, const int32 SetID)
{
    for (FAbilityTargetSet& Set : CurrentTargets)
    {
        if (Set.SetID == SetID)
        {
            Set.Targets.Remove(Target);
            return;
        }
    }
}

bool UCombatAbility::GetTargetSetByID(const int32 SetID, FAbilityTargetSet& OutTargetSet) const
{
    OutTargetSet.Targets.Empty();
    OutTargetSet.SetID = 0;
    for (const FAbilityTargetSet& Set : CurrentTargets)
    {
        if (Set.SetID == SetID)
        {
            OutTargetSet = Set;
            return true;
        }
    }
    return false;
}

#pragma endregion 
#pragma region Modifiers

void UCombatAbility::AddMaxChargeModifier(const FCombatModifier& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticMaxCharges || !IsValid(Modifier.Source))
    {
        return;
    }
    MaxChargeModifiers.Add(Modifier.Source, Modifier);
    RecalculateMaxCharges();
}

void UCombatAbility::RemoveMaxChargeModifier(const UBuff* Source)
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
    const int32 PreviousMaxCharges = AbilityCooldown.MaxCharges;
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
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

void UCombatAbility::AddChargeCostModifier(const FCombatModifier& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargeCost || !IsValid(Modifier.Source))
    {
        return;
    }
    ChargeCostModifiers.Add(Modifier.Source, Modifier);
    RecalculateChargeCost();
}

void UCombatAbility::RemoveChargeCostModifier(const UBuff* Source)
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
    const int32 PreviousCost = ChargeCost;
    TArray<FCombatModifier> Modifiers;
    ChargeCostModifiers.GenerateValueArray(Modifiers);
    ChargeCost = FCombatModifier::ApplyModifiers(Modifiers, DefaultChargeCost);
    if (ChargeCost != PreviousCost)
    {
        UpdateCastable();
    }
}

void UCombatAbility::AddChargesPerCooldownModifier(const FCombatModifier& Modifier)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || bStaticChargesPerCooldown || !IsValid(Modifier.Source))
    {
        return;
    }
    ChargesPerCooldownModifiers.Add(Modifier.Source, Modifier);
    RecalculateChargesPerCooldown();
}

void UCombatAbility::RemoveChargesPerCooldownModifier(const UBuff* Source)
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

void UCombatAbility::AddResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier)
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

void UCombatAbility::RemoveResourceCostModifier(const TSubclassOf<UResource> ResourceClass, UBuff* Source)
{
    if (OwningComponent->GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Source))
    {
        return;
    }
    TArray<FCombatModifier*> Modifiers;
    ResourceCostModifiers.MultiFindPointer(ResourceClass, Modifiers);
    bool bFound = false;
    for (const FCombatModifier* Mod : Modifiers)
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
        OwningComponent->CalculateCastLength(this) : DefaultCastTime;
}

float UCombatAbility::GetGlobalCooldownLength()
{
    return !bStaticGlobalCooldownLength && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateGlobalCooldownLength(this) : DefaultGlobalCooldownLength;
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
    else if (bTagsRestricted)
    {
        Castable = ECastFailReason::CustomRestriction;
    }
    else
    {
        Castable = ECastFailReason::None;
    }
}

void UCombatAbility::AddRestrictedTag(const FGameplayTag RestrictedTag)
{
    if (!RestrictedTag.IsValid() || (!RestrictedTag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityClassRestriction) && !AbilityTags.HasTag(RestrictedTag)))
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

void UCombatAbility::RemoveRestrictedTag(const FGameplayTag RestrictedTag)
{
    if (!RestrictedTag.IsValid() || (!RestrictedTag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityClassRestriction) && !AbilityTags.HasTag(RestrictedTag)))
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

void UCombatAbility::ActivateCastRestriction(const FGameplayTag RestrictionTag)
{
    if (!RestrictionTag.IsValid() || !RestrictionTag.MatchesTag(FSaiyoraCombatTags::Get().AbilityRestriction) || RestrictionTag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityRestriction))
    {
        return;
    }
    const int32 PreviousRestrictions = CustomCastRestrictions.Num();
    CustomCastRestrictions.Add(RestrictionTag);
    if (PreviousRestrictions == 0 && CustomCastRestrictions.Num() > 0)
    {
        UpdateCastable();
    }
}

void UCombatAbility::DeactivateCastRestriction(const FGameplayTag RestrictionTag)
{
    if (!RestrictionTag.IsValid() || !RestrictionTag.MatchesTag(FSaiyoraCombatTags::Get().AbilityRestriction) || RestrictionTag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityRestriction))
    {
        return;
    }
    const int32 PreviousRestrictions = CustomCastRestrictions.Num();
    CustomCastRestrictions.Remove(RestrictionTag);
    if (PreviousRestrictions > 0 && CustomCastRestrictions.Num() == 0)
    {
        UpdateCastable();
    }
}

#pragma endregion 