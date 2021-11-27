#include "SaiyoraBuffLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "Buff.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"


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