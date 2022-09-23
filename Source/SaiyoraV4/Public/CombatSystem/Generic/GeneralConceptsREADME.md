<a name="top"></a>
**[Back to Root](/README.md)**

# General Concepts

There are a few patterns that I found myself using in a lot of places in the combat system's code, and wanted to document since they pop up in nearly every component involved in combat.

## Combat Modifiers

A large number of actions players can take in combat result in calculating a value that can be affected by various modifiers from stats, buffs, or other sources. Modifiers can be either Additive or Multiplicative, and can optionally stack on themselves if sourced from a buff.  

The order of modifier application when calculating a value:
- Additive modifiers are summed (any additive modifier that stacks will be multiplied by its source buff's stack count before being added to the sum).
- Multiplicative modifiers are multiplied together (any multiplicative modifier will have 1 subtracted from it, then be multiplied by its source buff's stack count, then have 1 added back to it, then be clamped above 0).
- The base value will have the Additive sum added to it, then be clamped above 0, then be multiplied by the Multiplicative product, then be clamped above 0 again.

> As an example, take an ability with non-static cast length, a default cast length of 5 seconds, and with a stacking additive modifier that increases cast time by .5 seconds per stack at 3 stacks, and a stacking multiplicative modifier that reduces cast time by 10% per stack at 2 stacks.
>
> - Default cast length: 5 seconds
> - Additive Sum: .5 * 3 = 1.5
> - Multiplicative Product: (.9 - 1) * 2 + 1 = .8 (clamped above 0 for no effect)
> - Default (5) + Additive (1.5) = 6.5 (clamped above 0 for no effect)
> - 6.5 * .8 = 5.2 (clamped above zero for no effect)
> - Final cast length: 5.2 seconds  

## Modifiable Values  

Modifiable values are represented as one of two structs: FModifiableFloat or FModifiableInt. Since these are editor-exposed structs, templating them would have resulted in considerably more work than just having two near-identical alternatives. Each of these structs takes a default value, set in the editor or in a class' constructor in the case of C++-inherited classes, as well as an optional bModifiable flag that determines whether the value can actually be modified during gameplay. Their primary functionality is to hold a list of modifiers and constantly update a calculated value any time a modifier is added, removed, or updated. They also have an optional callback function that can be called any time the calculated value changes. They also support replication of the calculated value to trigger callbacks on the client.  

The current modifiable values are:  

> Ability Charge Cost  
> Ability Charges Per Cooldown  
> Ability Max Charges  

## Conditional Modifier Functions

Some modifiers to calculated values are not intended to always apply, and instead have a set of conditions that must be true to be factored in to a calculation. In these cases, components hold an array of functions that each take a specific event type as context and return a modifier, which may be invalid if the conditions aren't met. These functions are wrapped in Blueprint-exposed dynamic delegates, so that Blueprint functions may also be passed in as conditional modifier functions.

> As an example, dealing damage uses conditional modifier functions. If a player has a buff applied that increases Fire damage done by 20%, that is represented as a conditional modifier function that takes a damage event struct as context, and would look something like this: 
```
FCombatModifier FireDamageBuff(const FHealthEventInfo& EventInfo) 
{ 
    if (EventInfo.EventType == EHealthEventType::Damage && EventInfo.School == EHealthEventSchool::Fire)
    {
        return FCombatModifier(1.2f, EModifierType::Multiplicative);
    }
    return FCombatModifier();
} 
```
> When iterating over conditional modifiers to outgoing health events, this function would return a Multiplicative modifier of value 1.2 for Fire damage, and an invalid modifier for all other health events. Invalid modifiers are ignored entirely during calculation.

Conditional modifier functions are limited to values that need to be calculated with the context of an event, and as such, persistent values like stats do not support conditional modifier functions. They simply hold a collection of combat modifiers and are recalculated only when new modifiers are added, existing modifiers are removed, or existing modifiers change their stack count.  

Conditional modifiers are handled using the templated TConditionalModifierList, which simply holds a set of conditional modifier functions and has a GetModifiers function that takes in event context and returns all modifiers given that context.

Here is the list of all values that currently support conditional modifier functions:  

> Incoming Health Event Magnitude  
> Outgoing Health Event Magnitude    
> Incoming Threat Magnitude  
> Outgoing Threat Magnitude  
> Ability Cast Length  
> Ability Cooldown Length  
> Global Cooldown Length  
> Resource Delta Magnitude  

And here are the remaining values that only support simple modifiers but have not yet been refactored to use FModifiableInt/FModifiableFloat:  

> Ability Resource Cost  
> Stat Value  

## Conditional Restriction Functions

Similar in implementation and purpose to conditional modifier functions, conditional restriction functions represent a restriction for a specific event. These functions take an event struct as context, exactly like conditional modifier functions, but simply return a boolean that represents whether the event should be restricted or not. They are also wrapped in Blueprint-exposed delegates to allow Blueprint functions to be passed in as conditional restriction functions.

> As an example, a buff that prevents an actor from dying to attacks that deal less than 100 damage would apply a conditional restriction function like this:
```
bool PreventDeathToSmallDamage(const FHealthEvent& Event) 
{ 
    return Event.Result.AppliedValue < 100.0f; 
}
```
> When iterating over applied death restrictions, this function would return true for a damage event that dealt less than 100 damage, and would restrict death from occuring.  

The list of events that support conditional restriction functions:  

> Incoming Health Event  
> Outgoing Health Event  
> Death  
> Incoming Threat Event  
> Outgoing Threat Event  
> Interrupting  
> Incoming Buff Application  
> Outgoing Buff Application  
> External Movement Application  

## Prediction IDs

Used mostly within the context of the ability system, Prediction IDs are integers that represent an ability usage initiated by a non-server client. Actors controlled on the server (NPCs and players who are hosting as a listen server) always use 0 as a prediction ID. Prediction IDs are used to update predicted structs like the global cooldown, individual ability cooldowns, resource values, and cast status and keep things in sync between server and owning client. Abilities also typically pass their prediction ID to any buffs, projectiles, or predictively spawned actors, in order to sync things like movement stats or handle cleanup in the case of mispredictions.

**[⬆ Back to Top](#top)**
