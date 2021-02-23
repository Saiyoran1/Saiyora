// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAbilityHandler.h"
#include "ResourceHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"

void UPlayerAbilityHandler::BeginPlay()
{
    Super::BeginPlay();

    //TODO: Cast ResourceHandler to PlayerResourceHandler.
}

FCastEvent UPlayerAbilityHandler::UseAbility(::TSubclassOf<UCombatAbility> const AbilityClass)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        return Super::UseAbility(AbilityClass);
    }
    FCastEvent Event;
    if (GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return Event;
    }
    
    Event.Ability = FindActiveAbility(AbilityClass);
    if (!IsValid(Event.Ability))
    {
        Event.FailReason = FString(TEXT("Did not find valid active ability."));
        return Event;
    }

    if (!Event.Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
    {
        Event.FailReason = FString(TEXT("Cannot activate while dead."));
        return Event;
    }

    if (Event.Ability->GetHasGlobalCooldown() && GetGlobalCooldownState().bGlobalCooldownActive)
    {
        Event.FailReason = FString(TEXT("Already on global cooldown."));
        return Event;
    }

    if (GetCastingState().bIsCasting)
    {
        //TODO: Predict Cancel. Probably put this check later on? Separate this out into a check BEFORE attempting the cast?
        Event.FailReason = FString(TEXT("Already casting."));
        return Event;
    }

    TArray<FAbilityCost> Costs;
    Event.Ability->GetAbilityCosts(Costs);
    if (Costs.Num() > 0)
    {
        if (!IsValid(PlayerResourceHandler))
        {
            Event.FailReason = FString(TEXT("No valid resource handler found."));
            return Event;
        }
        if (!PlayerResourceHandler->CheckAbilityCostsMet(Event.Ability, Costs))
        {
            Event.FailReason = FString(TEXT("Ability costs not met."));
            return Event;
        }
    }

    if (!Event.Ability->CheckChargesMet())
    {
        Event.FailReason = FString(TEXT("Charges not met."));
        return Event;
    }

    if (!Event.Ability->CheckCustomCastConditionsMet())
    {
        Event.FailReason = FString(TEXT("Custom cast conditions not met."));
        return Event;
    }

    if (CheckAbilityRestricted(Event.Ability))
    {
        Event.FailReason = FString(TEXT("Cast restriction returned true."));
        return Event;
    }
    
    if (IsValid(CrowdControlHandler))
    {
        if (CrowdControlHandler->GetActiveCcTypes().HasAny(Event.Ability->GetRestrictedCrowdControls()))
        {
            Event.FailReason = FString(TEXT("Cast restricted by crowd control."));
            return Event;
        }
    }

    GenerateNewCastID(Event);

    if (Event.Ability->GetHasGlobalCooldown())
    {
        StartGlobalCooldown(Event.Ability, Event.CastID);
    }

    Event.Ability->CommitCharges(Event.CastID);
	
    if (Costs.Num() > 0 && IsValid(PlayerResourceHandler))
    {
        //TODO: PlayerResourceHandler PredictAbilityCost
        //PlayerResourceHandler->CommitAbilityCosts(Event.Ability, Event.CastID, Costs);
    }

    switch (Event.Ability->GetCastType())
    {
    case EAbilityCastType::None :
        Event.FailReason = FString(TEXT("No cast type was set."));
        return Event;
    case EAbilityCastType::Instant :
        Event.ActionTaken = ECastAction::Success;
        Event.Ability->InitialTick();
        //TODO: Gather parameters for the server.
        //TODO: Need to override the behavior in the multicast implementations of these functions.
        //BroadcastAbilityStart(Result);
        //BroadcastAbilityComplete(Result);
        break;
    case EAbilityCastType::Channel :
        Event.ActionTaken = ECastAction::Success;
        if (Event.Ability->GetHasInitialTick())
        {
            //TODO: Gather parameters for the server.
            Event.Ability->InitialTick();
        }
        //TODO: Predict Cast Start.
        //StartCast(Result.Ability, Result.CastID);
        //BroadcastAbilityStart(Result);
        break;
    default :
        Event.FailReason = FString(TEXT("Defaulted on cast type."));
        return Event;
    }

    //TODO: Call server UseAbility.
    
    return Event;
}

void UPlayerAbilityHandler::GenerateNewCastID(FCastEvent& CastEvent)
{
    if (GetOwnerRole() == ROLE_AutonomousProxy)
    {
        PredictionCastID++;
        CastEvent.CastID = PredictionCastID;
        return;
    }
    CastEvent.CastID = PredictionCastID;
    PredictionCastID = 0;
}

void UPlayerAbilityHandler::StartGlobalCooldown(UCombatAbility* Ability, int32 const CastID)
{
    if (GetOwnerRole() != ROLE_AutonomousProxy)
    {
        Super::StartGlobalCooldown(Ability, CastID);
        return;
    }
    FGlobalCooldown const PreviousGlobal = PredictedGlobal;
    PredictedGlobal.bGlobalCooldownActive = true;
    PredictedGlobal.CastID = CastID;
    if (PreviousGlobal.bGlobalCooldownActive != PredictedGlobal.bGlobalCooldownActive)
    {
        OnGlobalCooldownChanged.Broadcast(PreviousGlobal, PredictedGlobal);
    }
}

void UPlayerAbilityHandler::RollBackFailedGlobal(int32 const FailID)
{
    if (PredictedGlobal.bGlobalCooldownActive && PredictedGlobal.CastID == FailID)
    {
        FGlobalCooldown const PreviousGlobal = PredictedGlobal;
        PredictedGlobal.bGlobalCooldownActive = false;
        if (PreviousGlobal.bGlobalCooldownActive != PredictedGlobal.bGlobalCooldownActive)
        {
            OnGlobalCooldownChanged.Broadcast(PreviousGlobal, PredictedGlobal);
        }
    }
}

void UPlayerAbilityHandler::OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal)
{
    if (GetOwnerRole() != ROLE_AutonomousProxy)
    {
        Super::OnRep_GlobalCooldownState(PreviousGlobal);
        return;
    }
    if (GlobalCooldownState.CastID >= PredictedGlobal.CastID)
    {
        FGlobalCooldown const PreviousState = PredictedGlobal;
        PredictedGlobal = GlobalCooldownState;
        if (PreviousState.bGlobalCooldownActive != PredictedGlobal.bGlobalCooldownActive || PreviousState.StartTime != PredictedGlobal.StartTime || PreviousState.EndTime != PredictedGlobal.EndTime)
        {
            OnGlobalCooldownChanged.Broadcast(PreviousState, PredictedGlobal);
        }
    }
}
