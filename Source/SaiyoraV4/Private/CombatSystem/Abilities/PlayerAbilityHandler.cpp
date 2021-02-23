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

    //TODO: Do I need PlayerAbility class for this to work? Predicted Charges? Or can I calculate this within the component?
    //Could store all charge predictions in the component, but I'd need to update them whenever abilities replicate their charges.
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
        PredictGlobalCooldown(Event.Ability, Event.CastID);
    }

    //TODO: Predict Charge Expenditure
    //Event.Ability->CommitCharges(Result.CastID);
	
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

void UPlayerAbilityHandler::PredictGlobalCooldown(UCombatAbility* Ability, int32 const CastID)
{
    //TODO: GCD Prediction.
    return;
}

