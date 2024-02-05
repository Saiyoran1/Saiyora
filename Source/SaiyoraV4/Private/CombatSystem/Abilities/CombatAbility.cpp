#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "Resource.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraMovementComponent.h"
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
    DOREPLIFETIME_CONDITION(UCombatAbility, ChargesPerCooldown, COND_OwnerOnly);
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

    if (!bCastableWhileDead)
    {
        DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(OwningComponent->GetOwner());
        if (IsValid(DamageHandlerRef))
        {
            DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UCombatAbility::OnLifeStatusChanged);   
        }
    }

    if (!bCastableWhileMoving)
    {
        MovementCompRef = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(OwningComponent->GetOwner());
        if (IsValid(MovementCompRef))
        {
            MovementCompRef->OnMovementChanged.AddDynamic(this, &UCombatAbility::OnMovementChanged);
        }
    }

    if (!bCastableWhileCasting)
    {
        OwningComponent->OnCastStateChanged.AddDynamic(this, &UCombatAbility::OnCastStateChanged);
    }

    if (bOnGlobalCooldown)
    {
        OwningComponent->OnGlobalCooldownChanged.AddDynamic(this, &UCombatAbility::OnGlobalCooldownChanged);
    }

    FModifiableIntCallback MaxChargeCallback;
    MaxChargeCallback.BindUObject(this, &UCombatAbility::OnMaxChargesUpdated);
    MaxCharges.SetMinClamp(true, 1);
    AbilityCooldown.MaxCharges = FMath::Max(1, MaxCharges.GetCurrentValue());
    AbilityCooldown.CurrentCharges = AbilityCooldown.MaxCharges;
    MaxCharges.SetUpdatedCallback(MaxChargeCallback);
    
    FModifiableIntCallback ChargeCostCallback;
    ChargeCostCallback.BindUObject(this, &UCombatAbility::OnChargeCostUpdated);
    ChargeCost.SetUpdatedCallback(ChargeCostCallback);
    ChargeCost.SetMinClamp(true, 0);
    
    ChargesPerCooldown.SetMinClamp(true, 0);
    
    SetupResourceCosts();
    
    SetupCustomCastRestrictions();
    PreInitializeAbility();
    bInitialized = true;
    UpdateCastable();
}

void UCombatAbility::SetupCostCheckingForNewResource(UResource* Resource)
{
    if (UninitializedResources.Contains(Resource->GetClass()))
    {
        OnResourceValueChanged(Resource, nullptr, FResourceState(),
            FResourceState(Resource->GetMinimum(), Resource->GetMaximum(), Resource->GetCurrentValue()));
        Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::OnResourceValueChanged);
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
    if (OwningComponent->GetOwnerRole() != ROLE_Authority)
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
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        if (PredictionID != 0)
        {
            AbilityCooldown.PredictionID = PredictionID;
        }
        AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges - GetChargeCost(), 0, AbilityCooldown.MaxCharges);
        if (!AbilityCooldown.OnCooldown && AbilityCooldown.CurrentCharges < AbilityCooldown.MaxCharges)
        {
            //If prediction ID is non-zero, we are not locally controlled, so we should use lag compensation.
            StartCooldown(PredictionID != 0);
        }
    }
    else if (OwningComponent->GetOwnerRole() == ROLE_AutonomousProxy)
    {
        ChargePredictions.Add(PredictionID, GetChargeCost());
        RecalculatePredictedCooldown();
    }
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        UpdateCastable();
        OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
    }
}

void UCombatAbility::StartCooldown(const bool bUseLagCompensation)
{
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetServerWorldTimeSeconds();
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        AbilityCooldown.bAcked = true;
        float CooldownLength = OwningComponent->CalculateCooldownLength(this);
        if (bUseLagCompensation)
        {
            //Since there is no cooldown length prediction, we reduce cooldowns started from predicted ability usage (not from rolling over) by the actor's ping, up to a reasonable max.
            CooldownLength = CooldownLength - (FMath::Min(UAbilityComponent::MaxLagCompensation, USaiyoraCombatLibrary::GetActorPing(OwningComponent->GetOwner())));
        }
        AbilityCooldown.CooldownEndTime = AbilityCooldown.CooldownStartTime + CooldownLength;
        GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UCombatAbility::CompleteCooldown, CooldownLength, false);
    }
    else
    {
        AbilityCooldown.bAcked = false;
        AbilityCooldown.CooldownEndTime = -1.0f;
    }
}

void UCombatAbility::CompleteCooldown()
{
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.CurrentCharges = FMath::Clamp(AbilityCooldown.CurrentCharges + GetChargesPerCooldown(), 0, AbilityCooldown.MaxCharges);
    if (PreviousCharges != AbilityCooldown.CurrentCharges)
    {
        UpdateCastable();
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
        UpdateCastable();
        OnChargesChanged.Broadcast(this, PreviousState.CurrentCharges, AbilityCooldown.CurrentCharges);
    }
}

#pragma endregion
#pragma region Cost

void UCombatAbility::GetAbilityCosts(TArray<FSimpleAbilityCost>& OutCosts) const
{
    for (const FAbilityCost& AbilityCost : AbilityCosts.Items)
    {
        OutCosts.Add(FSimpleAbilityCost(AbilityCost.ResourceClass, AbilityCost.Cost.GetCurrentValue()));
    }
}

void UCombatAbility::SetupResourceCosts()
{
    AbilityCosts.OwningAbility = this;
    for (FAbilityCost& AbilityCost : AbilityCosts.Items)
    {
        const FModifiableFloatCallback CostChangeCallback = FModifiableFloatCallback::CreateLambda([&](const float OldValue, const float NewValue)
        {
            OnResourceCostChanged(AbilityCost);
        });
        AbilityCost.Cost.SetUpdatedCallback(CostChangeCallback);
        AbilityCost.Cost.SetMinClamp(true, 0.0f);
        bool bValidResource = false;
        if (IsValid(OwningComponent->GetResourceHandlerRef()))
        {
            UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(AbilityCost.ResourceClass);
            if (IsValid(Resource))
            {
                Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::OnResourceValueChanged);
                bValidResource = true;
                UpdateCostMet(AbilityCost.ResourceClass, Resource->GetCurrentValue() >= AbilityCost.Cost.GetCurrentValue());
            }
        }
        if (!bValidResource)
        {
            UninitializedResources.Add(AbilityCost.ResourceClass);
            UpdateCostMet(AbilityCost.ResourceClass, AbilityCost.Cost.GetCurrentValue() <= 0.0f);
        }
    }
    if (UninitializedResources.Num() > 0 && IsValid(OwningComponent->GetResourceHandlerRef()))
    {
        OwningComponent->GetResourceHandlerRef()->OnResourceAdded.AddDynamic(this, &UCombatAbility::SetupCostCheckingForNewResource);
    }
}

void UCombatAbility::OnResourceCostChanged(FAbilityCost& AbilityCost)
{
    AbilityCosts.MarkItemDirty(AbilityCost);
    UpdateCostMet(AbilityCost.ResourceClass, GetResourceValue(AbilityCost.ResourceClass) >= AbilityCost.Cost.GetCurrentValue());
}

void UCombatAbility::OnResourceValueChanged(UResource* Resource, UObject* ChangeSource,
                                                        const FResourceState& PreviousState, const FResourceState& NewState)
{
    UpdateCostMet(Resource->GetClass(), NewState.CurrentValue >= GetResourceCost(Resource->GetClass()));
}

float UCombatAbility::GetResourceCost(const TSubclassOf<UResource> ResourceClass) const
{
    for (const FAbilityCost& AbilityCost : AbilityCosts.Items)
    {
        if (AbilityCost.ResourceClass == ResourceClass)
        {
            return AbilityCost.Cost.GetCurrentValue();
        }
    }
    return 0.0f;
}

float UCombatAbility::GetResourceValue(const TSubclassOf<UResource> ResourceClass) const
{
    if (IsValid(OwningComponent->GetResourceHandlerRef()))
    {
        const UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(ResourceClass);
        if (IsValid(Resource))
        {
            return Resource->GetCurrentValue();
        }
    }
    return 0.0f;
}

void UCombatAbility::UpdateCostMet(const TSubclassOf<UResource> ResourceClass, const bool bMet)
{
    if (bMet)
    {
        if (UnmetCosts.Remove(ResourceClass) > 0 && UnmetCosts.Num() == 0)
        {
            UpdateCastable();
        }
    }
    else
    {
        const int32 PreviouslyUnmet = UnmetCosts.Num();
        UnmetCosts.Add(ResourceClass);
        if (PreviouslyUnmet == 0 && UnmetCosts.Num() > 0)
        {
            UpdateCastable();
        }
    }
}

void UCombatAbility::UpdateCostFromReplication(const FAbilityCost& Cost, const bool bNewAdd)
{
    if (IsValid(Cost.ResourceClass))
    {
        if (bNewAdd)
        {
            bool bValidResource = false;
            if (IsValid(OwningComponent->GetResourceHandlerRef()))
            {
                UResource* Resource = OwningComponent->GetResourceHandlerRef()->FindActiveResource(Cost.ResourceClass);
                if (IsValid(Resource))
                {
                    Resource->OnResourceChanged.AddDynamic(this, &UCombatAbility::OnResourceValueChanged);
                    UpdateCostMet(Cost.ResourceClass, Resource->GetCurrentValue() >= Cost.Cost.GetCurrentValue());
                    bValidResource = true;
                }
            }
            if (!bValidResource)
            {
                const int32 PreviouslyUninitialized = UninitializedResources.Num();
                UninitializedResources.Add(Cost.ResourceClass);
                if (PreviouslyUninitialized == 0 && UninitializedResources.Num() == 1)
                {
                    if (IsValid(OwningComponent->GetResourceHandlerRef()))
                    {
                        OwningComponent->GetResourceHandlerRef()->OnResourceAdded.AddDynamic(this, &UCombatAbility::SetupCostCheckingForNewResource);
                    }
                }
                UpdateCostMet(Cost.ResourceClass, Cost.Cost.GetCurrentValue() <= 0.0f);
            }
        }
        else
        {
            UpdateCostMet(Cost.ResourceClass, GetResourceValue(Cost.ResourceClass) >= Cost.Cost.GetCurrentValue());
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
        ChargePredictions.Add(Result.PredictionID, Result.ChargesSpent);
        RecalculatePredictedCooldown();
        if (PreviousCharges != AbilityCooldown.CurrentCharges)
        {
            UpdateCastable();
            OnChargesChanged.Broadcast(this, PreviousCharges, AbilityCooldown.CurrentCharges);
        }
    }
    if (!Result.bSuccess)
    {
        OnMisprediction(Result.PredictionID, Result.FailReasons);
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
    TArray<AActor*> NewTargetArray;
    NewTargetArray.Add(Target);
    AddTargetSet(FAbilityTargetSet(SetID, NewTargetArray));
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

FCombatModifierHandle UCombatAbility::AddMaxChargeModifier(const FCombatModifier& Modifier)
{
    return OwningComponent->GetOwnerRole() == ROLE_Authority ? MaxCharges.AddModifier(Modifier) : FCombatModifierHandle::Invalid;
}

void UCombatAbility::RemoveMaxChargeModifier(const FCombatModifierHandle& ModifierHandle)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        MaxCharges.RemoveModifier(ModifierHandle);
    }
}

void UCombatAbility::UpdateMaxChargeModifier(const FCombatModifierHandle& ModifierHandle,
    const FCombatModifier& NewModifier)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        MaxCharges.UpdateModifier(ModifierHandle, NewModifier);
    }
}

void UCombatAbility::OnMaxChargesUpdated(const int32 OldValue, const int32 NewValue)
{
    const int32 PreviousMaxCharges = AbilityCooldown.MaxCharges;
    const int32 PreviousCharges = AbilityCooldown.CurrentCharges;
    AbilityCooldown.MaxCharges = FMath::Max(1, NewValue);
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

FCombatModifierHandle UCombatAbility::AddResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority && IsValid(ResourceClass))
    {
        for (FAbilityCost& AbilityCost : AbilityCosts.Items)
        {
            if (AbilityCost.ResourceClass == ResourceClass)
            {
                return AbilityCost.Cost.AddModifier(Modifier);
            }
        }
    }
    return FCombatModifierHandle::Invalid;
}

void UCombatAbility::RemoveResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority && IsValid(ResourceClass))
    {
        for (FAbilityCost& AbilityCost : AbilityCosts.Items)
        {
            if (AbilityCost.ResourceClass == ResourceClass)
            {
                AbilityCost.Cost.RemoveModifier(ModifierHandle);
                return;
            }
        }
    }
}

void UCombatAbility::UpdateResourceCostModifier(const TSubclassOf<UResource> ResourceClass,
    const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority && IsValid(ResourceClass))
    {
        for (FAbilityCost& AbilityCost : AbilityCosts.Items)
        {
            if (AbilityCost.ResourceClass == ResourceClass)
            {
                AbilityCost.Cost.UpdateModifier(ModifierHandle, Modifier);
                return;
            }
        }
    }
}

float UCombatAbility::GetCastLength()
{
    return !bStaticCastTime ? OwningComponent->CalculateCastLength(this) : DefaultCastTime;
}

float UCombatAbility::GetGlobalCooldownLength()
{
    return !bStaticGlobalCooldownLength ? OwningComponent->CalculateGlobalCooldownLength(this) : DefaultGlobalCooldownLength;
}

float UCombatAbility::GetCooldownLength()
{
    return !bStaticCooldownLength ? OwningComponent->CalculateCooldownLength(this) : DefaultCooldownLength;
}

FCombatModifierHandle UCombatAbility::AddChargeCostModifier(const FCombatModifier& Modifier)
{
    return OwningComponent->GetOwnerRole() == ROLE_Authority ? ChargeCost.AddModifier(Modifier) : FCombatModifierHandle::Invalid;
}

void UCombatAbility::RemoveChargeCostModifier(const FCombatModifierHandle& ModifierHandle)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        ChargeCost.RemoveModifier(ModifierHandle);
    }
}

void UCombatAbility::UpdateChargeCostModifier(const FCombatModifierHandle& ModifierHandle,
    const FCombatModifier& NewModifier)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        ChargeCost.UpdateModifier(ModifierHandle, NewModifier);
    }
}

FCombatModifierHandle UCombatAbility::AddChargesPerCooldownModifier(const FCombatModifier& Modifier)
{
    return OwningComponent->GetOwnerRole() == ROLE_Authority ? ChargesPerCooldown.AddModifier(Modifier) : FCombatModifierHandle::Invalid;
}

void UCombatAbility::RemoveChargesPerCooldownModifier(const FCombatModifierHandle& ModifierHandle)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        ChargesPerCooldown.RemoveModifier(ModifierHandle);
    }
}

void UCombatAbility::UpdateChargesPerCooldownModifier(const FCombatModifierHandle& ModifierHandle,
    const FCombatModifier& NewModifier)
{
    if (OwningComponent->GetOwnerRole() == ROLE_Authority)
    {
        ChargesPerCooldown.UpdateModifier(ModifierHandle, NewModifier);
    }
}

#pragma endregion
#pragma region Restrictions

void UCombatAbility::UpdateCastable()
{
    const bool bWasCastable = CastFailReasons.Num() == 0;
    CastFailReasons.Empty();
    
    if (!bInitialized || bDeactivated)
    {
        CastFailReasons.AddUnique(ECastFailReason::InvalidAbility);
    }
    if (GetCurrentCharges() < GetChargeCost())
    {
        CastFailReasons.AddUnique(ECastFailReason::ChargesNotMet);
    }
    if (UnmetCosts.Num() > 0)
    {
        CastFailReasons.AddUnique(ECastFailReason::CostsNotMet);
    }
    if (CustomCastRestrictions.Num() > 0)
    {
        CastFailReasons.AddUnique(ECastFailReason::AbilityConditionsNotMet);
    }
    if (bTagsRestricted)
    {
        CastFailReasons.AddUnique(ECastFailReason::CustomRestriction);
    }
    if (!bCastableWhileCasting && OwningComponent->IsCasting())
    {
        CastFailReasons.AddUnique(ECastFailReason::AlreadyCasting);
    }
    if (bOnGlobalCooldown && OwningComponent->IsGlobalCooldownActive())
    {
        CastFailReasons.AddUnique(ECastFailReason::OnGlobalCooldown);
    }
    if (!bCastableWhileDead && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
    {
        CastFailReasons.AddUnique(ECastFailReason::Dead);
    }
    if (!bCastableWhileMoving && IsValid(MovementCompRef) && MovementCompRef->IsMoving())
    {
        CastFailReasons.AddUnique(ECastFailReason::Moving);
    }
    
    if (bWasCastable != (CastFailReasons.Num() == 0))
    {
        OnCastableChanged.Broadcast(this, CastFailReasons.Num() == 0, CastFailReasons);
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