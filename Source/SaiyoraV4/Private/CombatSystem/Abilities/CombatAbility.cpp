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
}

float UCombatAbility::GetCooldown() const
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

void UCombatAbility::StartCooldown()
{
    FTimerDelegate CooldownDelegate;
    CooldownDelegate.BindUFunction(this, FName(TEXT("CompleteCooldown")));
    float CooldownLength = DefaultCooldown;
    if (!bStaticCooldown)
    {
        float AddMod = 0.0f;
        float MultMod = 1.0f;
        for (FCombatModifier const& Mod : CooldownModifiers)
        {
            switch (Mod.ModifierType)
            {
                case EModifierType::Invalid :
                    break;
                case EModifierType::Additive :
                    AddMod += Mod.ModifierValue;
                    break;
                case EModifierType::Multiplicative :
                    MultMod *= FMath::Max(0.0f, Mod.ModifierValue);
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
    if (IsValid(AbilityComponent))
    {
        OwningComponent = AbilityComponent;
    }
    OnInitialize();
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

void UCombatAbility::OnRep_OwningComponent()
{
    if (!IsValid(OwningComponent))
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
