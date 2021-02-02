// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "ResourceHandler.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

void UCombatAbility::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

float UCombatAbility::GetCooldown() const
{
    return CooldownHandle.IsValid() ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f;
}

void UCombatAbility::CommitCharges()
{
    int32 const PreviousCharges = GetCurrentCharges();
    AbilityCooldown.CurrentCharges = FMath::Clamp((PreviousCharges - ChargesPerCast), 0, GetMaxCharges());
    if (PreviousCharges != GetCurrentCharges())
    {
        OnChargesChanged.Broadcast(this, PreviousCharges, GetCurrentCharges());
    }
    //TODO: Start cooldown if necessary.
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
        UE_LOG(LogTemp, Warning, TEXT("Ability Component pointer was not valid inside of a replicated ability on the client."));
        return;
    }
    OwningComponent->NotifyOfReplicatedAddedAbility(this);
}

void UCombatAbility::OnRep_AbilityCooldown()
{
    
}

void UCombatAbility::OnRep_MaxCharges()
{
    
}

void UCombatAbility::OnRep_ChargesPerCast()
{
    
}

void UCombatAbility::OnRep_CustomCastConditionsMet()
{
    
}
