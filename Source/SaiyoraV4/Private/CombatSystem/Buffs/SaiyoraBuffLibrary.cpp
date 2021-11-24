// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraBuffLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"

FBuffApplyEvent USaiyoraBuffLibrary::ApplyBuff(
    TSubclassOf<UBuff> const BuffClass,
    AActor* const AppliedBy,
    AActor* const AppliedTo,
    UObject* const Source,
    bool const DuplicateOverride,
    EBuffApplicationOverrideType const StackOverrideType,
    int32 const OverrideStacks,
    EBuffApplicationOverrideType const RefreshOverrideType,
    float const OverrideDuration,
    bool const IgnoreRestrictions,
    TArray<FCombatParameter> const& BuffParams)
{
    //Create the event struct.
    FBuffApplyEvent Event;

    //Check required parameters (from, to, source, buff class) for null.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source) || !IsValid(BuffClass))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }
    
    if (AppliedBy->GetLocalRole() != ROLE_Authority)
    {
        return Event;
    }

    //Check that the target implements the combat interface.
    if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }
    
    //Find a buff container component on the target.
    UBuffHandler* BuffTarget = ISaiyoraCombatInterface::Execute_GetBuffHandler(AppliedTo);
    if (!IsValid(BuffTarget))
    {
       Event.Result.ActionTaken = EBuffApplyAction::Failed;
       return Event;
    }

    //Fill the struct here so that condition checks have all the information.
    Event.AppliedBy = AppliedBy;
    Event.AppliedTo = AppliedTo;
    Event.Source = Source;
    Event.BuffClass = BuffClass;
    Event.DuplicateOverride = DuplicateOverride;
    Event.StackOverrideType = StackOverrideType;
    Event.OverrideStacks = OverrideStacks;
    Event.RefreshOverrideType = RefreshOverrideType;
    Event.OverrideDuration = OverrideDuration;
    if (Event.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(Event.AppliedBy);
        Event.AppliedByPlane = IsValid(AppliedByPlaneComp) ? AppliedByPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
    }
    else
    {
        Event.AppliedByPlane = ESaiyoraPlane::None;
    }
    UPlaneComponent* AppliedToPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(Event.AppliedTo);
    Event.AppliedToPlane = IsValid(AppliedToPlaneComp) ? AppliedToPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
    Event.AppliedXPlane = UPlaneComponent::CheckForXPlane(Event.AppliedByPlane, Event.AppliedToPlane);
    Event.CombatParams = BuffParams;

    //Incoming condition checks. Ignore restrictions will bypass these checks.
    if (!IgnoreRestrictions && BuffTarget->CheckIncomingBuffRestricted(Event))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

    UBuffHandler* BuffGenerator = nullptr;
    //Find a buff generator component on the actor applying the buff. Generator is not required.
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(AppliedBy);
    }

    //Outgoing restriction checks. Ignore restrictions being true or no valid buff generator being found will bypass these checks.
    if (!IgnoreRestrictions && IsValid(BuffGenerator) && BuffGenerator->CheckOutgoingBuffRestricted(Event))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

    //Buff container will dictate the actual application of the buff (either spawning a new one, stacking, refreshing, or failing).
    BuffTarget->ApplyBuff(Event);

    //If the event was successful, and the actor applying the buff did have a buff generator, notify that component of the event.
    if (Event.Result.ActionTaken == EBuffApplyAction::NewBuff && IsValid(BuffGenerator))
    {
        BuffGenerator->SuccessfulOutgoingBuffApplication(Event);
    }
    
    return Event;
}

FBuffRemoveEvent USaiyoraBuffLibrary::RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason)
{
    FBuffRemoveEvent Event;

    //Null buff will result in a failed removal.
    if (!IsValid(Buff) || !IsValid(Buff->GetAppliedTo()))
    {
        Event.Result = false;
        return Event;
    }

    if (Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
    {
        Event.Result = false;
        return Event;
    }

    //Check to make sure there is a valid buff container to remove the buff from.
    if (!Buff->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        Event.Result = false;
        return Event;
    }
    UBuffHandler* RemoveFrom = ISaiyoraCombatInterface::Execute_GetBuffHandler(Buff->GetAppliedTo());
    if (!IsValid(RemoveFrom))
    {
        Event.Result = false;
        return Event;
    }

    //Fill the struct here so we can proceed with removing the buff.
    Event.RemovedBuff = Buff;
    Event.AppliedBy = Buff->GetAppliedBy();
    Event.RemovedFrom = Buff->GetAppliedTo();
    Event.ExpireReason = ExpireReason;

    //Buff container will handle finding and removing the buff.
    RemoveFrom->RemoveBuff(Event);

    //On a successful removal, find the buff generator (if one exists) of the actor who applied the buff and notify it.
    if (Event.Result)
    {
        if (IsValid(Event.AppliedBy) && Buff->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
        {
            UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(Event.AppliedBy);
            if (IsValid(BuffGenerator))
            {
                BuffGenerator->SuccessfulOutgoingBuffRemoval(Event);
            }
        }
    }

    return Event;
}