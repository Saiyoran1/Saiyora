// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "ResourceHandler.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StatHandler.h"

const FGameplayTag UCombatAbility::CooldownLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false);
const float UCombatAbility::MinimumCooldownLength = 0.5f;

void UCombatAbility::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatAbility, OwningComponent);
    DOREPLIFETIME(UCombatAbility, bDeactivated);
    DOREPLIFETIME_CONDITION(UCombatAbility, AbilityCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, MaxCharges, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, ChargesPerCast, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UCombatAbility, bCustomCastConditionsMet, COND_OwnerOnly);
}

float UCombatAbility::GetRemainingCooldown() const
{
    return CooldownHandle.IsValid() ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f;
}

void UCombatAbility::CommitCharges(int32 const CastID)
{
    int32 const PreviousCharges = GetCurrentCharges();
    AbilityCooldown.CurrentCharges = FMath::Clamp((PreviousCharges - ChargesPerCast), 0, GetMaxCharges());
    if (CastID > 0)
    {
        AbilityCooldown.LastAckedID = CastID;
    }
    if (PreviousCharges != GetCurrentCharges())
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, GetCurrentCharges());
    }
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownHandle) && GetCurrentCharges() < MaxCharges)
    {
        StartCooldown();
    }
}

float UCombatAbility::GetAbilityCost(FGameplayTag const& ResourceTag) const
{
    if (!ResourceTag.IsValid() || !ResourceTag.MatchesTag(UResourceHandler::GenericResourceTag))
    {
        return -1.0f;
    }
    FAbilityCost const* Cost = AbilityCosts.Find(ResourceTag);
    if (Cost)
    {
        return Cost->Cost;
    }
    return -1.0f;
}

void UCombatAbility::StartCooldown()
{
    FTimerDelegate CooldownDelegate;
    CooldownDelegate.BindUFunction(this, FName(TEXT("CompleteCooldown")));
    float CooldownLength = DefaultCooldown;
    if (!bStaticCooldown)
    {
        float AddMod = 0.0f;
        float MultMod = 1.0f;
        FCombatModifier TempMod;
        for (FAbilityModCondition const& Condition : CooldownMods)
        {
            TempMod = Condition.Execute(this);
            switch (TempMod.ModifierType)
            {
                case EModifierType::Invalid :
                    break;
                case EModifierType::Additive :
                    AddMod += TempMod.ModifierValue;
                    break;
                case EModifierType::Multiplicative :
                    MultMod *= FMath::Max(0.0f, TempMod.ModifierValue);
                    break;
                default :
                    break;
            }
        }
        if (IsValid(OwningComponent->GetStatHandlerRef()))
        {
            float const ModFromStat = OwningComponent->GetStatHandlerRef()->GetStatValue(CooldownLengthStatTag);
            if (ModFromStat > 0.0f)
            {
                MultMod *= ModFromStat;
            }
        }
        CooldownLength = FMath::Max(0.0f, DefaultCooldown + AddMod) * MultMod;
    }
    CooldownLength = FMath::Max(CooldownLength, MinimumCooldownLength);
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, CooldownDelegate, CooldownLength, false);
    AbilityCooldown.OnCooldown = true;
    AbilityCooldown.CooldownStartTime = OwningComponent->GetGameStateRef()->GetWorldTime();
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
}

void UCombatAbility::GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const
{
    AbilityCosts.GenerateValueArray(OutCosts);
}

void UCombatAbility::InitializeAbility(UAbilityComponent* AbilityComponent)
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
    
    OnInitialize();
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

void UCombatAbility::InitialTick()
{
    OnInitialTick();
}

void UCombatAbility::NonInitialTick(int32 const TickNumber)
{
    OnNonInitialTick(TickNumber);
}

void UCombatAbility::CompleteCast()
{
    OnCastComplete();
}

void UCombatAbility::InterruptCast()
{
    OnCastInterrupted();
}

void UCombatAbility::CancelCast()
{
    OnCastCancelled();
}

void UCombatAbility::AddCooldownModifier(FAbilityModCondition const& Mod)
{
    if (!Mod.IsBound() || bStaticCooldown)
    {
        return;
    }
    CooldownMods.AddUnique(Mod);
}

void UCombatAbility::RemoveCooldownModifier(FAbilityModCondition const& Mod)
{
    CooldownMods.RemoveSingleSwap(Mod);
}

void UCombatAbility::AddCastTimeModifier(FAbilityModCondition const& Mod)
{
    if (!Mod.IsBound() || bStaticCastTime)
    {
        return;
    }
    CastTimeMods.AddUnique(Mod);
}

void UCombatAbility::RemoveCastTimeModifier(FAbilityModCondition const& Mod)
{
    CastTimeMods.RemoveSingleSwap(Mod);
}

void UCombatAbility::OnRep_OwningComponent()
{
    if (!IsValid(OwningComponent) || bInitialized)
    {
        return;
    }
    OwningComponent->NotifyOfReplicatedAddedAbility(this);
}

void UCombatAbility::OnRep_AbilityCooldown()
{
    return;
}

void UCombatAbility::OnRep_MaxCharges()
{
    return;
}

void UCombatAbility::OnRep_ChargesPerCast()
{
    return;
}

void UCombatAbility::OnRep_CustomCastConditionsMet()
{
    return;
}

void UCombatAbility::OnRep_Deactivated(bool const Previous)
{
    if (bDeactivated && !Previous)
    {
        OnDeactivate();
        OwningComponent->NotifyOfReplicatedRemovedAbility(this);
    }
}
