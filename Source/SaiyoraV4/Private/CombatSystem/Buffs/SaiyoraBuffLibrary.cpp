// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraBuffLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "Buff.h"

UBuffHandler* USaiyoraBuffLibrary::GetBuffHandler(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }
    return Target->FindComponentByClass<UBuffHandler>();
}

FBuffApplyEvent USaiyoraBuffLibrary::ApplyBuff(
    TSubclassOf<UBuff> const BuffClass,
    AActor* const AppliedBy,
    AActor* const AppliedTo,
    UObject* const Source,
    TArray<FBuffParameter> const EventParams,
    bool const DuplicateOverride,
    EBuffApplicationOverrideType const StackOverrideType,
    int32 const OverrideStacks,
    EBuffApplicationOverrideType const RefreshOverrideType,
    float const OverrideDuration,
    bool const IgnoreRestrictions)
{
    //Create the event struct.
    FBuffApplyEvent Event;

    //Check required parameters (from, to, source, buff class) for null.
    if (!AppliedBy || !AppliedTo || !Source || !BuffClass)
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

    //Find a buff container component on the target.
    UBuffHandler* BuffTarget = GetBuffHandler(AppliedTo);
    if (!BuffTarget)
    {
       Event.Result.ActionTaken = EBuffApplyAction::Failed;
       return Event;
    }

    //Fill the struct here so that condition checks have all the information.
    Event.AppliedBy = AppliedBy;
    Event.AppliedTo = AppliedTo;
    Event.Source = Source;
    Event.BuffClass = BuffClass;
    Event.EventParams = EventParams;
    Event.DuplicateOverride = DuplicateOverride;
    Event.StackOverrideType = StackOverrideType;
    Event.OverrideStacks = OverrideStacks;
    Event.RefreshOverrideType = RefreshOverrideType;
    Event.OverrideDuration = OverrideDuration;
    Event.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    Event.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    Event.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Event.AppliedByPlane, Event.AppliedToPlane);

    //Incoming condition checks. Ignore restrictions will bypass these checks.
    if (!IgnoreRestrictions && BuffTarget->CheckIncomingBuffRestricted(Event))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }
    
    //Find a buff generator component on the actor applying the buff. Generator is not required.
    UBuffHandler* BuffGenerator = GetBuffHandler(AppliedBy);

    //Outgoing restriction checks. Ignore restrictions being true or no valid buff generator being found will bypass these checks.
    if (!IgnoreRestrictions && BuffGenerator && BuffGenerator->CheckOutgoingBuffRestricted(Event))
    {
        Event.Result.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

    //Buff container will dictate the actual application of the buff (either spawning a new one, stacking, refreshing, or failing).
    BuffTarget->ApplyBuff(Event);

    //If the event was successful, and the actor applying the buff did have a buff generator, notify that component of the event.
    if (Event.Result.ActionTaken != EBuffApplyAction::Failed && BuffGenerator)
    {
        BuffGenerator->SuccessfulOutgoingBuffApplication(Event);
    }
    
    return Event;
}

FBuffRemoveEvent USaiyoraBuffLibrary::RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason)
{
    FBuffRemoveEvent Event;

    //Null buff will result in a failed removal.
    if (!Buff)
    {
        Event.Result = false;
        return Event;
    }

    //Check to make sure there is a valid buff container to remove the buff from.
    UBuffHandler* RemoveFrom = Buff->GetAppliedTo()->FindComponentByClass<UBuffHandler>();
    if (!RemoveFrom)
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
        UBuffHandler* BuffGenerator = Event.AppliedBy->FindComponentByClass<UBuffHandler>();
        if (BuffGenerator)
        {
            BuffGenerator->SuccessfulOutgoingBuffRemoval(Event);
        }
    }

    return Event;
}