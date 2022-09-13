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
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
    const bool bChargeCostPreviouslyMet = AbilityCooldown.CurrentCharges >= ChargeCost;
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        if (PredictionID != 0)
        {
            AbilityCooldown.PredictionID = PredictionID;
        }
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - ChargeCost, 0, AbilityCooldown.MaxCharges);
        if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
        {
            StartCooldown();
        }
    }
    else if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        ChargePredictions.Add(PredictionID, ChargeCost);
        RecalculatePredictedCooldown();
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        if (bChargeCostPreviouslyMet != (AbilityCooldown.CurrentCharges >= ChargeCost))
        {
            UpdateCastable();
        }
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
}

void UCombatAbility::StartCooldown()
{
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    AbilityCooldown.bAcked = OwningComponent->GetOwnerRole() == ROLE_Authority;
    const float CooldownLength = OwningComponent->GetOwnerRole() == ROLE_Authority ? OwningComponent->CalculateCooldownLength(this) : -1.0f;
    AbilityCooldown.CooldownEndTime = OwningComponent->GetOwnerRole() == ROLE_Authority ? AbilityCooldown.CooldownStartTime + CooldownLength : -1.0f;
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, CooldownLength, false);
    }
}

void UCombatAbility::CompleteCooldown()
{
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
    const bool bChargeCostPreviouslyMet = PreviousCharges >= ChargeCost;
    AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + ChargesPerCooldown, 0, AbilityCooldown.MaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        if (bChargeCostPreviouslyMet != (AbilityCooldown.CurrentCharges >= ChargeCost))
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
        CancelCooldown();
    }
}

void UCombatAbility::CancelCooldown()
{
    AbilityCooldown.OnCooldown = false;
    AbilityCooldown.CooldownStartTime = -1.0f;
    AbilityCooldown.CooldownEndTime = -1.0f;
    GetWorld()->GetTimerManager().ClearTimer(CooldownHandle);
}

void UCombatAbility::RecalculatePredictedCooldown()
{
    AbilityCooldown.CurrentCharges = LastReplicatedCharges;
    for (const TTuple<int32, int32>& Prediction : ChargePredictions)
    {
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - Prediction.Value, 0, AbilityCooldown.MaxCharges);
    }
    if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
    {
        StartCooldown();
    }
    else if (AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges == AbilityCooldown.MaxCharges)
    {
        CancelCooldown();
    }
}

void UCombatAbility::OnRep_AbilityCooldown(FAbilityCooldown const& PreviousState)
{
    LastReplicatedCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.bAcked = true;
    TArray<int32> OldIDs;
    for (const TTuple<int32, int32>& Prediction : ChargePredictions)
    {
        if (Prediction.Key <= AbilityCooldown.PredictionID)
        {
            OldIDs.Add(Prediction.Key);
        }
    }
    for (const int32 ID : OldIDs)
    {
        ChargePredictions.Remove(ID);
    }
    RecalculatePredictedCooldown();
    if (PreviousState.CurrentCharges != AbilityCooldown.CurrentCharges)
    {
        if ((PreviousState.CurrentCharges >= ChargeCost) != (AbilityCooldown.CurrentCharges >= ChargeCost))
        {
            UpdateCastable();
        }
        OnChargesChanged.Broadcast(this, PreviousState.CurrentCharges, AbilityCooldown.CurrentCharges);
    }
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
        const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
        const bool bPreviouslyMetChargeCost = AbilityCooldown.CurrentCharges >= ChargeCost;
        ChargePredictions.Add(Result.PredictionID, Result.ChargesSpent);
        RecalculatePredictedCooldown();
        if (PreviousCharges != AbilityCooldown.CurrentCharges)
        {
            if (bPreviouslyMetChargeCost != (AbilityCooldown.CurrentCharges >= ChargeCost))
            {
                UpdateCastable();
            }
            OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        }
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
    return !bStaticGlobalCooldownLength && IsValid(OwningComponent) ?
        OwningComponent->CalculateGlobalCooldownLength(this) : DefaultGlobalCooldownLength;
}

float UCombatAbility::GetCooldownLength()
{
    return OwningComponent->CalculateCooldownLength(this);
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