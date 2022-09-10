# Saiyora Technical Overview

_A quick note about GAS:_
_I did experiment with using Unreal Engine's Gameplay Ability System plugin for this project, but ultimately decided that creating my own ability system would give me more control over how certain behaviors like prediction and lag compensation were implemented, as well as more of an opportunity to understand the creation of such systems. Learning was a primary goal of the project alongside actually finishing a prototype of the game. I also disliked the way GAS handled certain concepts, like the disparity between abilities and buffs, the concept of ability cooldowns, and the usage of "meta attributes" like damage for calculated event values. Overall, I had a pretty clear idea of what limitations I wanted in place and didn't need all of the flexibility that GAS offered at the cost of an extremely generic framework, and preferred to go for a less flexible but more understandable (to me) system that I built myself._

---

## Generic Concepts

There are a few patterns that I found myself using in a lot of places in the combat system's code, and wanted to document since they pop up in nearly every component involved in combat.

### Combat Modifiers

Because of the influence of RPG and MMO-style games on Saiyora, I wanted to build a complex system of buffs, debuffs, and stats that could alter gameplay at runtime. A lot of actions you can take in combat result in calculating a value that determines how long a timer lasts, how effective an ability is, etc. To allow this, I set up a generic pattern of modifiers and a common system for handling them.

Buffs can apply modifiers to specific calculations (for example, Cast Length). These modifiers can be either Additive or Multiplicative, and can optionally stack on themselves (stacking is detailed in the Buffs section later). When calculating a value like Cast Length of an ability, there is a flag on the ability itself that denotes the value as Static or not. Static values will not be modified outside of clamping (for example, preventing a negative cast length). Non-static values will have modifiers applied to them.

The order of modifier application:
- Additive modifiers are summed (any additive modifier that stacks will be multiplied by its source's stack count before being added to the sum).
- Multiplicative modifiers are multiplied together (any multiplicative modifier will have 1 subtracted from it, then be multiplied by its source's stack count, then have 1 added back to it, then be clamped above 0).
- The base value will have the Additive sum added to it, then be clamped above 0, then be multiplied by the Multiplicative product, then be clamped above 0 again.

As an example, take an ability with non-static cast length, a default cast length of 5 seconds, and with a stacking additive modifier that increases cast time by .5 seconds at 3 stacks, and a stacking multiplicative modifier that reduces cast time by 10% at 2 stacks.

- Default cast length: 5 seconds
- Additive Sum: .5 * 3 = 1.5
- Multiplicative Product: (.9 - 1) * 2 + 1 = .8 (clamped above 0 for no effect)
- Default (5) + Additive (1.5) = 6.5 (clamped above 0 for no effect)
- 6.5 * .8 = 5.2 (clamped above zero for no effect)
- Final cast length: 5.2 seconds

This covers use cases for simple modifiers, for things like generically increasing all cast lengths by 20%. Some modifiable values currently only support simple modifiers, such as Max Charges, Charge Cost, Resource Cost, and Charges Per Cooldown (this is mostly to keep the updating of an ability's Castable status functioning correctly and displaying on the player UI correctly). 

### Conditional Modifiers

To get more specific modifier interactions, there is also the concept of conditional modifiers, which take effect only within specific contexts. Conditional modifiers are represented as functions that take an event as context and return a modifier to apply, which may be null. These functions are specific to the type of event they modify.

As an example, a modifier that reduces all direct fire damage taken by 20% would be a function that takes an FHealthEvent struct as context, checks if the HitStyle is Direct, checks if the School is Fire, and checks if the EventType is Damage. If any of those three are false, it would return an Invalid type modifier. If all three are true, it would return a Multiplicative modifier whose value is .8. Components hold an array of these delegates, and iterate over them, executing each one during calculation and storing the result in a modifier array, before using the generic modifier application calculation as above to apply them to a base value.

### Event Restrictions

Many events within the combat system may be conditionally restricted from being executed, including things like damage and healing, threat generation and threat actions, Plane swapping, and Buff application, among others. Taking a similar approach to conditional modifiers above, restrictions are also represented as functions that take an event struct as context and return a bool representing whether the event should be restricted.

As an example, a restriction that prevents all crowd control debuffs from being applied would be a function that takes an FBuffApplyEvent struct as context, checks if the buff class has a CrowdControl tag in its BuffTags, and if so returns true (preventing application), and if not returns false (allowing appication).

The ability system had need for a simpler system that wasn't dependent on context, to keep the Castable status of an ability updated correctly and UI display working properly, so custom cast restrictions and ability tag restrictions are used for ability activation instead of conditional restrictions. ([Ability Usage Conditions](#ability-usage-conditions))

### Prediction IDs

Used mostly within the context of the ability system, Prediction IDs are just integers that represent an ability usage initiated by a non-server client. Actors controlled on the server (NPCs and players who are hosting as a listen server) always use 0 as a prediction ID. Prediction IDs are used to update predicted structs like the GCD, ability cooldowns, resource values, and cast status and keep things in sync between server and owning client. Abilities also typically pass their prediction ID to any buffs, projectiles, or predictively spawned actors, in order to sync things like movement stats, or handle mispredictions.

---

## Abilities

Abilities are handled by the UAbilityComponent, which is a universal actor component that can be applied to players or NPCs to give them functionality to add, remove, and use objects derived from UCombatAbility, the generic ability/spell class in this game. For players, UAbilityComponent also handles prediction and reconciliation of the global cooldown timer, individual ability cooldowns, cast bars, resource costs, and any predicted parameters necessary to execute ability functionality, such as aim targets and camera location.  

### Ability Usage Flow

Ability usage is fairly complicated, especially when looking at prediction and replication. A basic overview from the perspective of a server-controlled actor (either an NPC or a player acting as a listen server) using an ability is as follows:

- Request ability use by TSubclassOf<UCombatAbility>.
- Check if the actor is locally controlled and the ability subclass is valid.
- Check active abilities to find one of the requested class.
- Check if the ability is usable. ([Ability Usage Conditions](#ability-usage-conditions))
- Start the global cooldown, if the ability is on-global. ([Global Cooldown](#global-cooldown))
- Commit the ability's charge cost and start the ability's cooldown if needed. ([Cooldowns and Charges](#cooldowns-and-charges))
- Commit any resource costs of the ability. ([Resources](#resources))
- For channeled abilities: Start a cast bar. ([Casting](#casting))
- For instant abilities OR channeled abilities with an initial tick: Execute the predicted tick and server tick functionality of the ability. ([Ability Functionality](#ability-functionality))
- If an ability tick was executed, multicast to execute the simulated tick on other machines.

For players that are not a listen server, things are more complicated, as there is a lot of prediction going on. Here is an overview of a predicted ability use by a non-server player:

ON CLIENT:
- Request ability use by TSubclassOf<UCombatAbility>.
- Check if the actor is locally controlled and the ability subclass is valid.
- Check active abilities to find one of the requested class.
- Check if the ability is usable. ([Ability Usage Conditions](#ability-usage-conditions))
- Generate a new Prediction ID.
- Start the global cooldown, if the ability is on-global. ([Global Cooldown](#global-cooldown))
- Commit the ability's charge cost and start the ability's cooldown if needed. ([Cooldowns and Charges](#cooldowns-and-charges))
- Commit any resource costs of the ability. ([Resources](#resources))
- For channeled abilities: Start a cast bar. ([Casting](#casting))
- For instant abilities OR channeled abilities with an initial tick: Execute the predicted tick functionality of the ability. ([Ability Functionality](#ability-functionality))
- Send ability request to the server.

ON SERVER:
- Check request for valid prediction ID.
- Perform all of the steps taken on the client, but with actual calculated values for cooldown length and cast bar length, and executing the server tick functionality instead of predicted tick functionality.
- If a tick was performed, multicast to execute simulated tick functionality on other clients.
- Notify the client of the result of the ability request.

ON CLIENT:
- Update resource cost predictions from server result.
- Update ability charge cost from server result.
- Update global cooldown from server result.
- In the case of a misprediction (the ability was denied by the server), call the ability's OnMispredicted functionality to clean up predicted effects.

<a name="ability-usage-conditions"></a>
### Ability Usage Conditions

Abilities need to be initialized on the server and owning client, and are not usable until this is done. Server initialization is done when adding the ability to an Ability Component. Client initialization is triggered by replication of a valid Ability Component reference within the ability instance. Abilities can also be deactivated when removed from an Ability Component, rendering them unusable and pending destruction.
  
Abilities have the option to be castable while dead, where they will ignore their owner's life status (if the owning actor even has a Damage Handler). In the event that the owner does not have a Damage Handler, this check is skipped entirely.

A number of ability-specific restrictions on usage are encapsulated in the Castable enum, which holds a reason that an ability is not castable at any given time, or None if the ability is castable. It is constantly updated by multiple different checks for the purpose of being displayable on the UI for players. These checks include:

- Charge cost check, updated any time the charge cost or current charges of the ability are updated. ([Cooldowns and Charges](#cooldowns-and-charges))
- Resource cost check, updated any time any of the resources the ability depends on change values (or are removed). ([Resources](#resources))
- Custom cast restrictions check, updated any time a custom cast restriction is activated or deactivated by the ability. These are for conditions that the ability imposes on itself, for example "Can only cast while moving," "Must have a debuff active on an enemy to cast," etc.
- Tag restrictions check, updated any time a restricted tag is added or removed from the ability. These are restrictions applied by sources other than the ability itself, including a generic restriction on an ability class (and all its derived classes) and more specific restrictions that are checked against an ability's tags.

Abilities marked as "On Global Cooldown" can not be activated during a global cooldown. ([Global Cooldown](#global-cooldown))

Abilities can not be activated during a channel of an ability. For players, there is some functionality setup inside the Player Character class (where ability input is initially handled) to automatically try and cancel a channel in progress to use a new ability if the channel isn't about to end (more on that in the Queueing section). ([Casting](#casting))

<a name="global-cooldown"></a>
### Global Cooldown

_Modifiable Values: Global Cooldown Length_

The global cooldown (GCD) is a concept commonly used in non-shooter games. Specifically, MMOs like World of Warcraft and Final Fantasy XIV use GCDs as a way to set the pace of combat, causing a short period after using most abilities where other abilities can not be used. Some abilities designated as "off-GCD" can ignore the GCD and not trigger its timer, but in the games I used as examples, the vast majority of core abilities trigger a GCD anywhere from 1 to 3 seconds. 

In Saiyora's combat system, most core damage and healing abilities trigger a GCD in the range of 1 to 1.5 seconds, to create a similar feel to combat systems you would find in RPGs where damage throughput comes mostly from selecting the correct ability at the right time rather than from spamming abilities as fast as possible. The most notable exceptions to this are weapons, which do NOT trigger a GCD (or have a cooldown), and instead manage fire rate separately (which is detailed more in the Weapons section later on). Additionally, most defensive, mobility, and utility skills are off-GCD, allowing them to be used fluidly and reactively as needed.

GCDs are client predicted in the Ability Component, including their length (which differs from the approach used to predict cooldowns and cast bars later). When a client activates an ability that triggers a GCD, they will calculate the GCD length locally and immediately start the GCD timer. In the case of misprediction, the local GCD timer is cancelled if it still active and still on the GCD started by the mispredicted ability use. In the case of success, the server will send the client its calculated GCD length as part of the response, in which case the client can use its own predicted ability start time to determine a new time for the GCD to end. 

There is potential for issues if the ping of the client drops in between ability uses, as a request for using a new ability might arrive before the previous GCD has ended on the server, despite the client locally seeing his predicted GCD complete. For this reason, the server sends the client the correct calculated GCD length, but actually sets its own GCD timer to a reduced value, based on a universal multiplier of the requesting client's ping (currently set at 20%), so that it would be exceedingly rare for a client's ping to fluctuate enough naturally to cause their next ability request to arrive before the previous GCD had finished on the server (without cheating or completely mispredicting). There is some potential for cheating here, as clients could send new ability requests up to 20% earlier by bypassing their local GCD timer, and the server would accept this as valid. One way to combat this would be to do some internal bookkeeping of GCD discrepancies over time, and if clients are consistently sending new ability use requests within that 20% window, disconnect them. However, I have not implemented this functionality right now.

<a name="cooldowns-and-charges"></a>
### Cooldowns and Charges

_Modifiable Values: Charge Cost, Cooldown Length, Charges per Cooldown_

Cooldowns in Saiyora are based on a charge system. This means every ability can be used a certain amount of times before running out of charges and needing to wait for the ability's cooldown timer to finish and refund some or all of those uses. A basic ability would have a single charge, and its cooldown would restore that single charge, which would mirror the functionality most commonly seen in RPGs or in Epic's own Gameplay Ability System. However, the charge system gives flexibility to design abilities that can also be fired multiple times before needing to cooldown, and even the ability for a cooldown to restore multiple charges to an ability, creating some resource management and "dump" windows for abilities where it is encouraged to use multiple charges in quick succession to avoid wasting refunded charges from the ability's cooldown.

Prediction of cooldowns is a little less robust than prediction of GCDs, as they are typically longer in length than GCD, and thus it is unlikely that an ability would go on cooldown and then have that cooldown complete within a window the length of the client's ping. They also can "roll over" in the case of multiple charge abilities that don't get all their charges back in one cooldown cycle, in which case my normal prediction ID system would need to be reworked to account for new cooldowns as a new prediction, despite no ability use triggering them. As such, charge expenditure is predicted when an ability is used, and the cooldown of the ability will "start" locally, however no timer will be displayed and length will not be calculated.

The server keeps track of the authoritative cooldown state, calculating charge cost and cooldown length when using an ability, then setting a replicated cooldown state struct in the ability itself for the owning client to use. On replication of the server's cooldown state, the client will then discard any outdated predictions (identified by prediction ID, which the cooldown state contains) and accept the server's cooldown progress. If there are remaining predictions, the client will then recalculate its predicted charges and start a new indefinite-length cooldown if needed. In the case of misprediction, similar steps are taken, with only the mispredicted charge expenditure being thrown out and then the cooldown progress being recalculated.

Currently on my list of TODOs: Adding in the same lag compensation window to cooldowns triggered by an ability use (NOT cooldowns triggered by "rolling over" at the end of a previous cooldown) that GCD uses, to prevent players with higher ping from being disadvantaged with longer cooldowns on abilities.

<a name="casting"></a>
### Casting

_Modifiable Values: Cast Length_

There are two types of abilities in Saiyora: instant and channeled. Instant abilities happen within a single frame, while channeled abilities happen over time, in a set number of "ticks" (note this is not Unreal Engine's game tick, but a term adopted from MMOs that represents an instance of an ability activation within a set triggered by a single cast) that are equally spaced throughout the ability's duration. Channeled abilities can either have an initial tick or not, and then have at least 1 non-initial tick where the ability's functionality is triggered, the last of which occurs at the end of the cast duration.

Since abilities are always instanced in Saiyora, they can keep internal state between ticks and between casts, and are also passed the tick number each time they activate (for instant casts this is always 0) to allow abilities that perform different actions throughout their duration. Channeled abilities will cause their caster to display a cast bar in the UI, showing progress through the cast.

Channeled abilities also have the ability to be cancelled and interrupted. Interrupting will have its own dedicated section later. Cancelling a channeled ability simply ends it and prevents any further ticks from happening. Both cancelling and interrupting can trigger behavior on the ability if needed through seperate OnInterrupted and OnCancelled functions.

Prediction of casts is handled similarly to cooldowns, where the predicting client will instantly trigger the initial tick (if the ability has one), and display an empty cast bar, but will not calculate the length of the cast or trigger any subsequent ticks until the server has responded with the correct cast length or notice of misprediction. Misprediction simply results in the immediate end of the cast (this is not considered a cancellation and will not trigger behavior), while confirmation of the cast length allows the client to use his predicted activation time to recreate the cast bar, trigger any ability ticks that may have been missed in the time between predicting and receiving confirmation of the ability usage, and set up a timer for the next tick. Each tick is predicted separately, with the client sending any necessary data at his own local tick intervals, and the server simply holding that data then triggering the tick at the right time (if it arrives before the server ticks), or marking the tick as lacking data and triggering it when client data arrives (if it arrives after the server ticks). There is a time limit on how far out of sync the server will execute a predicted tick to prevent the client from sending ticks at incorrect times and potentially causing issues. Currently, this limit is 1 tick, so the server will ignore a tick entirely if the next tick of the ability happens on the server without receiving prediction data. This may need to be adjusted if there is a need for a lot of fast-ticking abilities used by players (as NPCs don't need to worry about prediction).

<a name="player-ability-input"></a>
### Player Ability Input

Ability input for players is handled in the APlayerCharacter class, where there is some additional logic beyond just calling UseAbility on every new ability input. The main additional functionality includes:

- Cancelling channels on new input.
- Queueing input.
- Re-using automatically firing abilities.
- Cancelling "cancel on release" channels on release.

When ability input is received and another channel is already in progress, players will either cancel the current channel and then attempt to use the newly input ability, or, in the case of an input that occurs within a small window (currently .2 seconds) of the end of the channel, the player will "queue" the input until the end of the current channel and then reattempt it. This queueing logic also works for GCDs that are about to end, to prevent pressing an input a few milliseconds early from resulting in a failed ability use.

Abilities can be marked as Automatic, which means that holding their input will constantly re-use the ability when possible until another input is pressed. This currently runs on every frame if an automatic ability's input is held down, unless the player swaps Planes (switching what ability the input corresponds to) or has another ability in queue. Weapons are the main reason this functionality exists.

Abilities can also be marked as Cancel on Release, which means that their input must be held down to continue the channel. Releasing the input of a channeled ability that is Cancel on Release will immediately cancel the channel, unless it is within the queueing window, in which case the channel will be allowed to finish. This allows an early escape from abilities where the entire channel may not be desired.

<a name="ability-functionality"></a>
### Ability Functionality
  
  Abilities have functionality attached to a a multitude of different functions. The primary places where abilities perform their functionality are in the Tick functions: PredictedTick, ServerTick, and SimulatedTick. They can also have functionality tied to being cancelled, interrupted, and mispredicted, though the misprediction functionality should be limited to cleaning up predicted effects such as projectiles or particles.
  
- Predicted Tick runs on the locally controlled client, whether it is the server or not.
- Server Tick runs only on the server. This means that players on listen servers will fire both the Predicted and Server Ticks back-to-back.
- Simulated Tick runs only on clients that are not the server and not controlling the actor, and is intended to be used mostly for cosmetic effects.

Typically, Predicted Tick should be where any targeting or input-reliant logic is handled, with target data being passed along to the Server Tick automatically after being set. Server Tick should handle the authoritative logic that can not happen on clients, and can also have some conditional cosmetic effects to allow listen server players to see effects for other players' abilities. Simulated tick should just be used to pass results of the ability use to other clients and allow them to have cosmetic effects.

Cancelling has a similar set of functions (PredictedCancel, ServerCancel, SimulatedCancel), and Interrupting has a Server and Simulated version (Interrupts only happen on the server, and can not be predicted). Misprediction is not called on simulated clients, only on the server and the owning client, as it should really just be used to clean up predicted effects.
  
---

<a name="resources"></a>
## Resources
  

