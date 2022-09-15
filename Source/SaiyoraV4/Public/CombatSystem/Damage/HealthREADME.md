# Health

The UDamageHandler component handles health, death, respawning, kill count reporting, and both incoming and outgoing health events (damage, healing, and absorbs). Health events support a variety of options, including a hit style, an ability school, an optional source modifier, threat parameters, and flags to ignore restrictions, ignore modifiers, bypass absorbs, or use a snapshotted value. All health events and calculations take place exclusively on the server, with the resulting health values being replicated down and events being RPCed to involved clients. While most of the concepts concerning damage and healing are fairly straightforward, there are a few behaviors of note.

## Absorbs

Absorbs are essentially a bonus health pool. Some games call this concept shields, overshields, or overhealth. Any damage taken will eat through absorbs first before affecting the health pool, unless the bBypassAbsorbs flag is used. Absorbs in Saiyora don't automatically regenerate, default to a value of 0, and can be applied up to a value equivalent to the actor's max health. This means reductions in max health may also cause a reduction in absorb value. 

For players, dealing damage will only visually display the damage value dealt to actual health, so floating combat text will display zero damage done if absorbs are not broken. This is something I'd like to adjust in the future.

## Snapshotting

Health events support snapshotting, which is a term used to refer to effects that calculate their damage or healing value up front, and then re-use that value multiple times over a duration. Commonly seen in damage-over-time and healing-over-time effects, snapshotting promotes stacking damage modifiers before applying a long-duration damage effect, so that it will continue to deal increased damage from those modifiers after they have expired.

Snapshotting works by bypassing outgoing modifiers, with the assumption that those modifiers have already been accounted for in the base value provided for the event. This behavior is available on an event-by-event basis through the bFromSnapshot flag, to make marking some effects as snapshotted and others as dynamic possible. Health events include a "snapshot value" in their result struct so that persistent effects can easily access the original value for later applications.

## Source Modifiers

Health events have an optional source modifier parameter, which is a conditional modifier function that the source of the event can use to add additional behavior.

> As an example, an ability that deals 20% extra damage to targets under 50% health would provide a source modifier delegate referencing a function that looks like this:  
```
FCombatModifier DealBonusDamageToLowHealth(const FHealthEventInfo& EventInfo)
{
    UDamageHandler* TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(EventInfo.AppliedTo);
    if (IsValid(TargetHandler) && TargetHandler->GetCurrentHealth() / TargetHandler->GetMaxHealth() < 0.5f)
    {
        return FCombatModifier(1.2f, EModifierType::Multiplicative);
    }
    return FCombatModifier();
}
```
This can provide a very different result from an alternative approach where the ability checks this condition before applying damage and adjusts the base value to be passed in, since the source modifier is handled at the same time as other outgoing modifiers. Note that snapshotting bypasses any provided source modifier.

## Killing Blows  

Killing blows are any damage event that reduces an actor's health to zero and subsequently causes it to die. Since death is a restrictable event, not every instance of hitting zero health results in a killing blow. In cases where death is restricted for a given damage event, the event is saved off as a pending killing blow and the bHasPendingKillingBlow flag is set to true. Any time a death restriction is removed, the pending killing blow is re-checked to see if the actor is now allowed to die.

Any healing that brings the actor above zero health will clear the pending killing blow and associated flag. Absorbs do NOT trigger this behavior, since they do not actually change the actor's current health. New damage events that occur while a pending killing blow is already saved will not overwrite the original event, to allow the first instance of the actor hitting zero health to be correctly credited as the killing blow in the case of multiple damage events happening before a removed death restriction causes the actor to finally die.

When an actor dies, if it has a valid kill count type, it will call its associated death reporting function, which just notifies the GameState of the actor's death to trigger any game-wide behavior necessary.  

Right now, there are three supported report types in addition to "None."  

- "Player" reports add to the dungeon death count and invoke a small time penalty.  
- "Boss" reports mark a specific boss (with a provided GameplayTag) as killed, which fulfills one of the requirements of dungeon completion.  
- "Trash" reports add a provided kill count value to the dungeon's current kill count, which must reach a certain number to be considered complete.  

All of this functionality is handled internally in the GameState, so overriding GameState classes could use these reports differently in the future to support other modes outside of dungeon running.  

## Respawning

By default, actors don't support respawning (as most NPCs in the game won't respawn). Setting the bCanRespawn flag to true enables respawning with the option of respawning in place, or at a preset location. An actor's respawn location can be updated on the server at any time, and calling RespawnActor also allows the option of overriding the actor's set respawn point with a passed in location and rotation, as well as an optional health percentage to respawn at (where the default is full health).
