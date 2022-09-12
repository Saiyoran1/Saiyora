# Game Overview

Saiyora is a third person shooter game with RPG elements that focuses on speedrunning developer-created "dungeons" with a group of 1-4 players. It features a game world split into two alternate Planes: the modern and the ancient, which players can move between to alter their abilities and change the nature of combat. The modern plane allows players to focus on gunplay, with magical abilities that enhance the player's movement, defenses, damage, or healing, debuff enemies, or provide important utility. The ancient plane allows players to manifest magic as physical attacks, giving them access to a powerful arsenal of spells to fight with instead of their gun.

Currently, the game is planned to be completely PvE (player-vs-environment), with no PvP mode, though I would like to add a PvP mode in the future. It will support listen and dedicated servers, for casual dungeon running and ranked runs for a leaderboard respectively. The game focuses heavily on allowing players to build out characters, with two separate specializations (one for each Plane), each with their own talent system allowing customization on a per-dungeon basis. In the future, the game will support saving and sharing builds between players online.

The main influence on the design of the game was World of Warcraft's Challenge Mode system, which existed briefly during the Mists of Pandaria and Warlords of Draenor expansions. Though WoW is not a shooter, the overall nature of pushing for Challenge Mode times is something I hope to replicate. Some core takeaways from this system that I hope to translate into my own game:

- Infinite retries. Limiting attempts or otherwise restricting players from constantly being able to try very difficult strategies places a limit on the maximum amount of risk and complexity a dungeon run can realistically support. For more casual players, the cost of failure in a dungeon with limited retries means that groups often become toxic when mistakes are made, and progressing to more difficult strategies becomes discouraged.  

- Determinism within the dungeon. While some things will always be random (for example, physics in Unreal are not deterministic by default), limiting the effect of randomness on the outcome of a dungeon run is important to making sure that success is contingent on good strategy and execution, rather than handling of bad luck or maximizing of good luck. This means that NPC patrol pathing, spawning, ability priority, etc. should involve no randomness, and instead be based strictly on things like time and combat thresholds (like health percentage). It also means that player output should feature little randomness when given the same input.  

- Short goal times. Well-executed dungeon runs by skilled players should be between 5 and 10 minutes long. Casual runs should exist somewhere in the 15-20 minute range. Though poorly executed runs can and will exceed these goal times, players should never feel that their group played correctly and still had to spend half an hour or more in a single dungeon.  

- Lack of artificial caps on speed and difficulty. Things like mandatory cutscenes, dialogue, delays in spawning of certain enemies, or time-based progression blocks can add to the feel or story of a game, but in the case of speedrunning are mostly frustrating for players and inflate the necessary time to complete a dungeon. I also aim to reduce things like hard crowd control or interrupt requirements that limit the number of enemies that can be fought at once based sheerly on the available number of players in the dungeon, enemies that must be fought alone or in small groups due to either lack of movement, progression blocks, or overwhelming combat advantages that discourage larger fights, and any mechanic that unduly punishes players for fighting more enemies, beyond the inherent difficulty of having to deal with more enemy mechanics at a time. I firmly believe that fights involving a larger number of enemies, each with their own skillset to be dealt with, creates chaos that is fun to deal with and rewarding to overcome.  

A few notes about the game:

- I am currently the only developer on this project.  

- The game is made with Unreal Engine 5. Most underlying systems are coded in C++, with Blueprints being heavily used for animation, UI, AI, and content.  

- All art is placeholder. Almost every art asset is either a default UE5 asset, purchased from the Unreal marketplace, or downloaded from a free source such as Mixamo. Some UI elements and animations were created or modified by me.  

---

## Combat System Technical Overview

_A quick note about GAS:_
_I did experiment with using Unreal Engine's Gameplay Ability System plugin for this project, but ultimately decided that creating my own ability system would give me more control over how certain behaviors like prediction and lag compensation were implemented, as well as more of an opportunity to understand the creation of such systems. Learning was a primary goal of the project alongside actually finishing a prototype of the game. I also disliked the way GAS handled certain concepts, like the disparity between abilities and buffs, the concept of ability cooldowns, and the usage of "meta attributes" like damage for calculated event values. Overall, I had a pretty clear idea of what limitations I wanted in place and didn't need all of the flexibility that GAS offered at the cost of an extremely generic framework, and preferred to go for a less flexible but more understandable (to me) system that I built myself._

---

<a name="table-of-contents"></a>
## Table of Contents

> 1. [Generic Concepts](#generic-concepts)  
>   1.1 [Combat Modifiers](#combat-modifiers)  
>   1.2 [Conditional Modifiers](#conditional-modifiers)  
>   1.3 [Event Restrictions](#event-restrictions)  
>   1.4 [Prediction IDs](#prediction-ids)  
> 2. [Abilities](#abilities)  
>   2.1 [Ability Usage Flow](#ability-usage-flow)  
>   2.2 [Ability Usage Conditions](#ability-usage-conditions)  
>   2.3 [Global Cooldown](#global-cooldown)  
>   2.4 [Cooldowns and Charges](#cooldowns-and-charges)  
>   2.5 [Casting](#casting)  
>   2.6 [Player Ability Input](#player-ability-input)  
>   2.7 [Ability Functionality](#ability-functionality)  
> 3. [Resources](#resources)  
>   3.1 [Resource Initialization](#resource-initialization)  
>   3.2 [Resource Costs](#resource-costs)  
>   3.3 [Resource Delta Modifiers](#resource-delta-modifiers)  
> 4. [Stats](#stats)  
>   4.1 [Stat Initialization](#stat-initialization)  
>   4.2 [Stat Modifiers](#stat-modifiers)  
> 5. [Buffs](#buffs)  
>   5.1 [Buff Application](#buff-application)  
>   5.2 [Buff Initialization](#buff-initialization)  
>   5.3 [Buff Functions](#buff-functions)  
>   5.4 [Buff Removal](#buff-removal)
> 6. [Crowd Control](#crowd-control)
> 7. [Planes](#planes)  
>   7.1 [Modern Plane](#modern-plane)  
>   7.2 [Ancient Plane](#ancient-plane)  
>   7.3 [Plane Swapping](#plane-swapping)  
>   7.4 [Plane Attunement](#plane-attunement)  
> 8. [Movement](#movement)  
>   8.1 [Custom Moves](#custom-moves)  
>   8.2 [Root Motion](#root-motion)  

---

<a name="generic-concepts"></a>
## 1. Generic Concepts

There are a few patterns that I found myself using in a lot of places in the combat system's code, and wanted to document since they pop up in nearly every component involved in combat.

<a name="combat-modifiers"></a>
### 1.1 Combat Modifiers

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

<a name="conditional-modifiers"></a>
### 1.2 Conditional Modifiers

To get more specific modifier interactions, there is also the concept of conditional modifiers, which take effect only within specific contexts. Conditional modifiers are represented as functions that take an event as context and return a modifier to apply, which may be null. These functions are specific to the type of event they modify.

As an example, a modifier that reduces all direct fire damage taken by 20% would be a function that takes an FHealthEvent struct as context, checks if the HitStyle is Direct, checks if the School is Fire, and checks if the EventType is Damage. If any of those three are false, it would return an Invalid type modifier. If all three are true, it would return a Multiplicative modifier whose value is .8. Components hold an array of these delegates, and iterate over them, executing each one during calculation and storing the result in a modifier array, before using the generic modifier application calculation as above to apply them to a base value.

<a name="event-restrictions"></a>
### 1.3 Event Restrictions

Many events within the combat system may be conditionally restricted from being executed, including things like damage and healing, threat generation and threat actions, Plane swapping, and Buff application, among others. Taking a similar approach to conditional modifiers above, restrictions are also represented as functions that take an event struct as context and return a bool representing whether the event should be restricted.

As an example, a restriction that prevents all crowd control debuffs from being applied would be a function that takes an FBuffApplyEvent struct as context, checks if the buff class has a CrowdControl tag in its BuffTags, and if so returns true (preventing application), and if not returns false (allowing appication).

The ability system had need for a simpler system that wasn't dependent on context, to keep the Castable status of an ability updated correctly and UI display working properly, so custom cast restrictions and ability tag restrictions are used for ability activation instead of conditional restrictions. ([Ability Usage Conditions](#ability-usage-conditions))

<a name="prediction-ids"></a>
### 1.4 Prediction IDs

Used mostly within the context of the ability system, Prediction IDs are just integers that represent an ability usage initiated by a non-server client. Actors controlled on the server (NPCs and players who are hosting as a listen server) always use 0 as a prediction ID. Prediction IDs are used to update predicted structs like the GCD, ability cooldowns, resource values, and cast status and keep things in sync between server and owning client. Abilities also typically pass their prediction ID to any buffs, projectiles, or predictively spawned actors, in order to sync things like movement stats, or handle mispredictions.

---

<a name="abilities"></a>
## 2. Abilities

Abilities are handled by the UAbilityComponent, which is a universal actor component that can be applied to players or NPCs to give them functionality to add, remove, and use objects derived from UCombatAbility, the generic ability/spell class in this game. For players, UAbilityComponent also handles prediction and reconciliation of the global cooldown timer, individual ability cooldowns, cast bars, resource costs, and any predicted parameters necessary to execute ability functionality, such as aim targets and camera location.  

__Available Callbacks:__ _On Ability Added, On Ability Removed, On Ability Tick, On Ability Mispredicted, On Ability Cancelled, On Ability Interrupted, On Cast State Changed, On Global Cooldown Changed, On Charges Changed_  

__Modifiable Values:__ _Cast Length, Global Cooldown Length, Cooldown Length, Resource Cost, Max Charges, Charge Cost, Charges Per Cooldown_  

__Restrictable Events:__ _Ability Use, Interrupt_  

<a name="ability-usage-flow"></a>
### 2.1 Ability Usage Flow

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
- Commit any resource costs of the ability. ([Resource Costs](#resource-costs))
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
### 2.2 Ability Usage Conditions

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
### 2.3 Global Cooldown

The global cooldown (GCD) is a concept commonly used in non-shooter games. Specifically, MMOs like World of Warcraft and Final Fantasy XIV use GCDs as a way to set the pace of combat, causing a short period after using most abilities where other abilities can not be used. Some abilities designated as "off-GCD" can ignore the GCD and not trigger its timer, but in the games I used as examples, the vast majority of core abilities trigger a GCD anywhere from 1 to 3 seconds. 

In Saiyora's combat system, most core damage and healing abilities trigger a GCD in the range of 1 to 1.5 seconds, to create a similar feel to combat systems you would find in RPGs where damage throughput comes mostly from selecting the correct ability at the right time rather than from spamming abilities as fast as possible. The most notable exceptions to this are weapons, which do NOT trigger a GCD (or have a cooldown), and instead manage fire rate separately (which is detailed more in the Weapons section later on). Additionally, most defensive, mobility, and utility skills are off-GCD, allowing them to be used fluidly and reactively as needed.

GCDs are client predicted in the Ability Component, including their length (which differs from the approach used to predict cooldowns and cast bars later). When a client activates an ability that triggers a GCD, they will calculate the GCD length locally and immediately start the GCD timer. In the case of misprediction, the local GCD timer is cancelled if it still active and still on the GCD started by the mispredicted ability use. In the case of success, the server will send the client its calculated GCD length as part of the response, in which case the client can use its own predicted ability start time to determine a new time for the GCD to end. 

There is potential for issues if the ping of the client drops in between ability uses, as a request for using a new ability might arrive before the previous GCD has ended on the server, despite the client locally seeing his predicted GCD complete. For this reason, the server sends the client the correct calculated GCD length, but actually sets its own GCD timer to a reduced value, based on a universal multiplier of the requesting client's ping (currently set at 20%), so that it would be exceedingly rare for a client's ping to fluctuate enough naturally to cause their next ability request to arrive before the previous GCD had finished on the server (without cheating or completely mispredicting). There is some potential for cheating here, as clients could send new ability requests up to 20% earlier by bypassing their local GCD timer, and the server would accept this as valid. One way to combat this would be to do some internal bookkeeping of GCD discrepancies over time, and if clients are consistently sending new ability use requests within that 20% window, disconnect them. However, I have not implemented this functionality right now.

<a name="cooldowns-and-charges"></a>
### 2.4 Cooldowns and Charges

Cooldowns in Saiyora are based on a charge system. This means every ability can be used a certain amount of times before running out of charges and needing to wait for the ability's cooldown timer to finish and refund some or all of those uses. A basic ability would have a single charge, and its cooldown would restore that single charge, which would mirror the functionality most commonly seen in RPGs or in Epic's own Gameplay Ability System. However, the charge system gives flexibility to design abilities that can also be fired multiple times before needing to cooldown, and even the ability for a cooldown to restore multiple charges to an ability, creating some resource management and "dump" windows for abilities where it is encouraged to use multiple charges in quick succession to avoid wasting refunded charges from the ability's cooldown.

Prediction of cooldowns is a little less robust than prediction of GCDs, as they are typically longer in length than GCD, and thus it is unlikely that an ability would go on cooldown and then have that cooldown complete within a window the length of the client's ping. They also can "roll over" in the case of multiple charge abilities that don't get all their charges back in one cooldown cycle, in which case my normal prediction ID system would need to be reworked to account for new cooldowns as a new prediction, despite no ability use triggering them. As such, charge expenditure is predicted when an ability is used, and the cooldown of the ability will "start" locally, however no timer will be displayed and length will not be calculated.

The server keeps track of the authoritative cooldown state, calculating charge cost and cooldown length when using an ability, then setting a replicated cooldown state struct in the ability itself for the owning client to use. On replication of the server's cooldown state, the client will then discard any outdated predictions (identified by prediction ID, which the cooldown state contains) and accept the server's cooldown progress. If there are remaining predictions, the client will then recalculate its predicted charges and start a new indefinite-length cooldown if needed. In the case of misprediction, similar steps are taken, with only the mispredicted charge expenditure being thrown out and then the cooldown progress being recalculated.

Currently on my list of TODOs: Adding in the same lag compensation window to cooldowns triggered by an ability use (NOT cooldowns triggered by "rolling over" at the end of a previous cooldown) that GCD uses, to prevent players with higher ping from being disadvantaged with longer cooldowns on abilities.

<a name="casting"></a>
### 2.5 Casting

There are two types of abilities in Saiyora: instant and channeled. Instant abilities happen within a single frame, while channeled abilities happen over time, in a set number of "ticks" (note this is not Unreal Engine's game tick, but a term adopted from MMOs that represents an instance of an ability activation within a set triggered by a single cast) that are equally spaced throughout the ability's duration. Channeled abilities can either have an initial tick or not, and then have at least 1 non-initial tick where the ability's functionality is triggered, the last of which occurs at the end of the cast duration.

Since abilities are always instanced in Saiyora, they can keep internal state between ticks and between casts, and are also passed the tick number each time they activate (for instant casts this is always 0) to allow abilities that perform different actions throughout their duration. Channeled abilities will cause their caster to display a cast bar in the UI, showing progress through the cast.

Channeled abilities also have the ability to be cancelled and interrupted. Interrupting will have its own dedicated section later. Cancelling a channeled ability simply ends it and prevents any further ticks from happening. Both cancelling and interrupting can trigger behavior on the ability if needed through seperate OnInterrupted and OnCancelled functions.

Prediction of casts is handled similarly to cooldowns, where the predicting client will instantly trigger the initial tick (if the ability has one), and display an empty cast bar, but will not calculate the length of the cast or trigger any subsequent ticks until the server has responded with the correct cast length or notice of misprediction. Misprediction simply results in the immediate end of the cast (this is not considered a cancellation and will not trigger behavior), while confirmation of the cast length allows the client to use his predicted activation time to recreate the cast bar, trigger any ability ticks that may have been missed in the time between predicting and receiving confirmation of the ability usage, and set up a timer for the next tick. Each tick is predicted separately, with the client sending any necessary data at his own local tick intervals, and the server simply holding that data then triggering the tick at the right time (if it arrives before the server ticks), or marking the tick as lacking data and triggering it when client data arrives (if it arrives after the server ticks). There is a time limit on how far out of sync the server will execute a predicted tick to prevent the client from sending ticks at incorrect times and potentially causing issues. Currently, this limit is 1 tick, so the server will ignore a tick entirely if the next tick of the ability happens on the server without receiving prediction data. This may need to be adjusted if there is a need for a lot of fast-ticking abilities used by players (as NPCs don't need to worry about prediction).

<a name="player-ability-input"></a>
### 2.6 Player Ability Input

Ability input for players is handled in the APlayerCharacter class, where there is some additional logic beyond just calling UseAbility on every new ability input. The main additional functionality includes:

- Cancelling channels on new input.
- Queueing input.
- Re-using automatically firing abilities.
- Cancelling "cancel on release" channels on release.

When ability input is received and another channel is already in progress, players will either cancel the current channel and then attempt to use the newly input ability, or, in the case of an input that occurs within a small window (currently .2 seconds) of the end of the channel, the player will "queue" the input until the end of the current channel and then reattempt it. This queueing logic also works for GCDs that are about to end, to prevent pressing an input a few milliseconds early from resulting in a failed ability use.

Abilities can be marked as Automatic, which means that holding their input will constantly re-use the ability when possible until another input is pressed. This currently runs on every frame if an automatic ability's input is held down, unless the player swaps Planes (switching what ability the input corresponds to) or has another ability in queue. Weapons are the main reason this functionality exists.

Abilities can also be marked as Cancel on Release, which means that their input must be held down to continue the channel. Releasing the input of a channeled ability that is Cancel on Release will immediately cancel the channel, unless it is within the queueing window, in which case the channel will be allowed to finish. This allows an early escape from abilities where the entire channel may not be desired.

<a name="ability-functionality"></a>
### 2.7 Ability Functionality
  
Abilities have functionality attached to a a multitude of different functions. The primary places where abilities perform their functionality are in the Tick functions: PredictedTick, ServerTick, and SimulatedTick. They can also have functionality tied to being cancelled, interrupted, and mispredicted, though the misprediction functionality should be limited to cleaning up predicted effects such as projectiles or particles.
  
- Predicted Tick runs on the locally controlled client, whether it is the server or not.
- Server Tick runs only on the server. This means that players on listen servers will fire both the Predicted and Server Ticks back-to-back.
- Simulated Tick runs only on clients that are not the server and not controlling the actor, and is intended to be used mostly for cosmetic effects.

Typically, Predicted Tick should be where any targeting or input-reliant logic is handled, with target data being passed along to the Server Tick automatically after being set. Server Tick should handle the authoritative logic that can not happen on clients, and can also have some conditional cosmetic effects to allow listen server players to see effects for other players' abilities. Simulated tick should just be used to pass results of the ability use to other clients and allow them to have cosmetic effects.

Cancelling has a similar set of functions (PredictedCancel, ServerCancel, SimulatedCancel), and Interrupting has a Server and Simulated version (Interrupts only happen on the server, and can not be predicted). Misprediction is not called on simulated clients, only on the server and the owning client, as it should really just be used to clean up predicted effects.
  
---

<a name="resources"></a>
## 3. Resources
  
Resources are things like mana or energy, that are spent and generated by abilities as part of the normal management of combat flow. They support prediction of ability costs, manual value modification on the server, linking to Stats for fluctuating max and min values, and custom max and min value initialization.
  
__Available Callbacks:__ _On Resource Added, On Resource Removed, On Resource Changed_  
  
__Modifiable Values:__ _Resource Delta_  
  
__Restrictable Events:__ _None_

<a name="resource-initialization"></a>
### 3.1 Resource Initialization
  
Resource initialization happens first on the server, with each UResource getting a reference to it's handler and it's owner's UStatHandler component (if one exists) for binding max and min values to a Stat. Each resource is also passed an initialization struct that contains an optional custom initial, custom max, and custom min values, which will override the resource class' default initial, max, and min values. Resources can not have negative values, and as such are clamped above 0 at all times.
  
Resource classes can optionally have a Minimum Bind Stat and Maximum Bind Stat set, in which case they will check that their owner has a valid Stat Handler, that handler has a valid value for the selected Stat, and then set their min or max value to the stat's value, subscribing to the stat's OnStatChanged delegate to adjust any time the stat is updated. Current value is adjusted any time max or min value changes, to keep it proportional to the new range of values the resource supports and clamped within that new range.
  
There is a also a BlueprintNative PreInitializeResource function called after initial setup, that allows resources to perform some logic on initialization if needed. This is useful for things like setting up resource regeneration or tracking owner interactions to modify resources outside of the usual costs of ability usage or gains from external calls to ModifyResource.

Client resource initialization is handled inside PostNetReceive, which checks for a valid Resource Handler ref, then calls PreInitializeResource client-side as well.

<a name="resource-costs"></a>
### 3.2 Resource Costs
  
Abilities have designated resource costs required to use them. These costs can be modified at runtime by adding Resource Cost Modifiers to the Ability Handler, and do not support conditional modifiers in order to accurately keep track of an ability's Castable status without having to check resource costs of every ability on tick to account for any new context. 
  
During ability usage, the ability class will call CommitAbilityCost. On the server, this just directly modifies the resource value, updates the latest prediction ID on the resource value struct, calls the OnResourceChanged delegate for other objects subscribed to the resource's behavior, and then calls PostResourceUpdated, which is another BlueprintNative function to allow any internal logic that may need to trigger from a resource changing (the best example of this once again being supporting passive regeneration of a resource).
  
On clients that are predicting ability usage, CommitAbilityCost adds a prediction struct that contains the prediction ID and predicted resource cost of the ability, then calls RecalculatePredictedResource, which updates a separate client resource value using the last replicated server value and any additional predictions the client has made. Something on my TODO list for the future is converting this into a system more similar to ability charges, where there isn't a need for a separate client value (and as such, no need to override the getter for the resource's value to change based on net role).
  
When the server sends back a response to a predicted ability usage, the updated costs replace the client's original prediction, and RecalculatePredictedResource is called again. When the server replicates the new value down through normal variable replication, all old predictions are cleared and the recalculation can then take into account only predictions newer than the replicated value.
  
<a name="resource-delta-modifiers"></a>
### 3.3 Resource Delta Modifiers
  
Though ability costs don't support conditional modifiers due to UI and performance concerns, other modification of resource values does. Resources have a ModifyResource function available on the server that allows manual changing of a resource's value, with the option to ignore modifiers or not. Conditional modifiers for resource deltas take the resource itself, the source object of the modification, and the amount to modify the resource by as context.
  
Right now there is no prediction for modifying resources, but it is something that could be supported in the future to handle things like reloading restoring ammo or abilities that instantly grant some resource and make gameplay a bit smoother in such cases. It's on the TODO list as well, but hasn't been a high priority due to reloading being the only current case where it would drastically improve the game experience under lag. This would need to be part of a larger change to Prediction IDs to allow them to carry ability tick number, and sync replicated variables more granularly by tick rather than just by ability usage.
  
---
  
<a name="stats"></a>
## 4. Stats
  
Stats are any value that needs to be modified at runtime for usage in combat calculation. This includes everything from max health modifiers to generic cast length modifiers, but notably does not replace things like max health VALUES or current health, which are concepts explicitly handled by the damage handler. This is one of the key differences between my approach and Epic's approach in GAS, where ANY modifiable value is handled as an attribute.
  
__Available Callbacks:__ _On Stat Changed_  
  
__Modifiable Values:__ _Stat Value_  
  
__Restrictable Events:__ _None_  
  
<a name="stat-initialization"></a>
### 4.1 Stat Initialization
  
Stats are initialized on the server using a DataTable that provides info on each stat's tag, default value, optional max and min clamps, replication needs, and modifiable status. Stat tags are GameplayTags starting with "Stat." and must be unique for each stat. Stats, like resources, currently do not support negative values, and as such are clamped above 0 even when no min clamp is specified. 
  
The Stat Handler keeps track of a list of static stats, replicated stats, and non-replicated stats on the server. Static and non-replicated stats are not replicated to clients to save on bandwidth, while replicated stats use a FFastArraySerializer setup to get individual stat callbacks on clients without having to instantiate each one as a UObject. Clients perform stat initialization independently of the server to have default values for all stats in the case that a stat value is needed for a static stat, or before replication of a stat's value has occurred.
  
<a name="stat-modifiers"></a>
### 4.2 Stat Modifiers
  
Stat modifiers can only be applied on the server, and any changes to stats are replicated down. Stats that are marked as static do not support modification, and also do not support subscription to their On Stat Changed callback. Stats do not support conditional modifiers, since there isn't really a valid context that would affect a modifier beyond "which stat is being modified."
  
Events often use stats as one of the modifiers in a calculation, taking the stat's current value and creating a Multiplicative modifier with it. This is a common approach in many components in the combat system, used in events ranging from cast length calculations to damage. As such, there is no concept of something like a "damage" stat, only a "damage done modifier" stat that represents a generic all-encompassing modifier to outgoing damage, and allowing more specific requirements to be represented in the Damage Handler itself as conditional modifiers.
  
One final thing to note is that there are no On Stat Added or On Stat Removed callbacks, since stats are only created in the Stat Handler's InitializeComponent function and not added or removed dynamically during gameplay. As such, there is a possible scenario where another component could try to subscribe to a stat that isn't yet replicated to a client. In this case, the FFastArraySerializer that contains replicated stats also keeps a TMultiMap of pending callbacks for not-yet-replicated stats, subscribing and notifying them when a stat is first replicated. This solves the race condition between stat replication and client-side subscription to stat values.
  
---
  
<a name="buffs"></a>
## 5. Buffs
  
Buffs are essentially any status effect that can be applied to an actor. The UBuffHandler component handles application and receiving of buffs, which are instanced as replicated UBuff objects. Buffs represent any change to combat state that happens for a duration of time, including modification to stats, restriction of events, periodic events like damage or healing over time, or threat actions.

__Available Callbacks:__ _On Incoming Buff Applied, On Incoming Buff Removed, On Outgoing Buff Applied, On Outgoing Buff Removed, On Updated, On Removed_  

__Modifiable Values:__ _None_  

__Restrictable Events:__ _Incoming Buff Application, Outgoing Buff Application_  

<a name="buff-application"></a>
### 5.1 Buff Application

Buff application is relatively complex, since buffs themselves have a variety of options for stacking, duration, overriding, and being restricted. Buffs in Saiyora are not predicted on clients, and can only be applied and removed on the server before being replicated down. 

When applying a buff, there are a few parameters passed in, including a Duplicate Override flag, a Stack Override type and value, a Refresh Override type and value, an Ignore Restrictions flag, and an array of Buff Parameters. The override flags are a way to bypass the default behavior of a buff class during gameplay when applying multiple buffs of the same class from the same actor. 

- Duplication is a binary behavior, where an application of a buff either creates a new instance each time it is applied with its own parameters, stacks, and duration, or whether it instead just modifies an already existing instance of the same class.  
- Stacking affects non-duplicating buffs by changing the value of an existing buff instance, usually by multiplying its effectiveness in some capacity or working toward some stack threshold that triggers new behavior. Stacking can be Additive or Overriding, and the amount of stacks to add or override with can also be passed in to bypass the buff's default stacking behavior.  
- Refreshing affects non-duplicating buffs by changing the remaining duration of an existing buff instance. This also features an Additive or Overriding option to either extend the duration by an amount or set the duration to a new value, bypassing the buff's default refreshing behavior.  

The Buff Handler also has a flag to never receive buffs, in the case of actors that need to apply buffs to other actors but shouldn't ever be considered a viable target of incoming buffs.

During application, an FBuffApplyEvent is generated that contains the buff class, passed parameters, target, source, applying actor, and Plane information of both the target and applying actor. Then, the Buff Handler uses this struct as context to check against any event restrictions on incoming buffs for the target or outgoing buffs from the applying actor. The Ignore Restrictions flag bypasses this step if true.

After restrictions are checked, the Buff Handler then checks for an existing buff of the same class with the same applying actor (currently, multiple actors can apply the same buff to a target with no repercussions by default, and to stop this behavior an event restriction would need to be used). If the buff is duplicable, or a duplicate override is passed in, or no buff of said class and owner is found, a new buff is created. Otherwise the existing buff is passed the FBuffApplyEvent to modify itself. 

Buffs that are modified fire off an On Updated callback inside the Buff instance, and modify the passed in FBuffApplyEvent struct to signify any changes to stacks or duration that occurred because of the application, which is used by the applying Buff Handler to fire off its own On Outgoing Buff Applied callback later. There is also a BlueprintNativeEvent that is called on each new application for any additional logic, and all Buff Functions owned by the buff are notified of any changes in stacks or duration.

<a name="buff-initialization"></a>
### 5.2 Buff Initialization

When a new buff is created on the server, initialization occurs in a few steps. The buff is passed a reference to its Handler to save, and also retrieves a reference to the GameState, which is needed for keeping track of time (which is important for handling refreshing and expiration). It will then initialize its stacks based on the override type (or lack of one) and value, the default initial stacks, and the maximum stacks the buff class supports. Buffs of finite duration (buffs can have infinite duration, in which case they must be manually removed) then set their application time and start their timer for expiration based on the refresh override type and value, default initial duration, and max duration. 

Buffs also store off their creation event in a replicated struct, that is subsequently used on clients to initialize the buff. This also allows buffs to check their original state, which could be used to affect the behavior of a buff over its lifetime. Buffs also store whether they were applied with the Ignore Restrictions flag, which will cause them to ignore any new restrictions added to the Buff Handler during their lifetime that would otherwise remove them. 

Buff functions, which are detailed below, are set up at this point. The buff then notifies its Handler that it is valid, triggering the Handler's On Incoming Buff Applied callback, before calling each buff function's On Apply behavior, and then the buff's BlueprintNative On Apply behavior.

Client initialization happens in the OnRep_CreationEvent function, which runs essentially the same code including buff function setup, notification of the Handler, and On Apply behavior. The buff then checks if another application event has been replicated down (this can sometimes happen if buffs are reapplied rapidly on the server before the client receives the initial creation event), and runs the logic for the latest application if necessary.

<a name="buff-functions"></a>
### 5.3 Buff Functions

Buff functions are reusable behaviors that buffs can instantiate and apply with custom parameters, for the purpose of avoiding writing common buff functionality in every buff that needs different values. Examples of buff functions that I have already implemented include:  

- Damage Over Time  
- Healing Over Time  
- Stat Modification  
- Crowd Control  
- Event Restrictions  
- Modifier Application  
- Threat Actions  

Buff function instantiation is set up in a way that isn't the most clear, but allows easy usage in Blueprint buffs inside of the BlueprintNative SetupCommonBuffFunctions function that is run during initialization. Each buff function is its own class derived from UBuffFunction, which contains callbacks for an owning buff's On Apply, On Stacked, On Refreshed, and On Removed events, as well as a SetupBuffFunction and CleanupBuffFunction for pre-initialization setup and post-removal clean up. Each buff function class declares a static BlueprintCallable function that creates an instance of the buff function, registers it with the owning buff, and passes any necessary parameters to a class-specific SetVariables function. From there, buff functions simply override the behavior callbacks that are triggered by the owning buff to do whatever it is they need.  

Because of the diverse nature of required parameters for buff functions, an alternate approach where buff functions are initialized from structs of function classes and parameters wasn't really feasible without creating an extremely generic parameter class, and even then, a lot of buff functions take function delegates as parameters (to enable conditional modifiers and event restrictions), which aren't supported currently as Blueprint variables in the editor beyond some limited functionality.

<a name="buff-removal"></a>
### 5.4 Buff Removal

Most buffs are simply removed when their timer expires, triggering their own On Removed callback as well as their Handler's On Incoming Buff Removed and their applier's On Outgoing Buff Removed. Buffs can also be removed manually by calling RemoveBuff on the Handler with the buff instance and an expiration reason (with the options Expired, Dispelled, Death, or Absolute). 

One interesting thing with buff removal is the interaction of event restrictions on already applied buffs. Any buff application restriction added to a Buff Handler will check any existing buffs that weren't originally applied with Ignore Restrictions, and remove them if they would now be restricted.

---

<a name="crowd-control"></a>
## 6. Crowd Control

Crowd control is a subset of status effects that remove capabilities from an actor. In Saiyora, all crowd control is applied through buffs, and there are six different types of crowd control.

- Stuns, which prevent essentially all actions (other than abilities that are marked as usable while stunned), including movement.  
- Roots, which prevent movement and immediately stop an actor's momentum, but do not prevent turning, crouching, or ability usage.  
- Incapacitates, which are identical to stuns but are removed upon receiving damage.  
- Silences, which prevent ability usage.  
- Disarms, which prevent usage of abilities derived from the base weapon class.  

Crowd control is handled by the UCrowdControlHandler component, which monitors buffs applied to its owning actor's UBuffHandler, checking their tags for any gameplay tag starting with "CrowdControl.". It maintains a structure representing the status of each of the six crowd control types, including whether they are active, which buffs are applying them, and which of the buffs applying the crowd control have the longest remaining duration (mostly for use in the UI, but also to reduce on checking whether removed buffs should cause crowd control to be removed). It also monitors damage taken by the owning actor's UDamageHandler to remove any buffs applying an Incapacitate effect on damage taken.

Crowd control application is not a restrictable event. The Crowd Control Handler does have a list of crowd control tags that it is immune to, but it is not modifiable during gameplay. Currently, the UBuffHandler actually manually checks during its application of a buff whether the UCrowdControlHandler exists and if its list of immuned crowd controls contains any tags that an applied buff has, and will restrict buff application entirely. I have gone back and forth on this approach vs having the UCrowdControlHandler apply a buff event restriction in BeginPlay, as either one works, but it is unlikely that I will change how crowd control immunity works and this system works fine.

__Available Callbacks:__ _On Crowd Control Changed_  

__Modifiable Values:__ _None_  

__Restrictable Events:__ _None_  

---

<a name="planes"></a>
## 7. Planes

Planes in Saiyora represent the two different eras of the world that are bleeding together. From a gameplay perspective, playing in each Plane is distinct because it swaps the set of abilities available to the player, provides interactions with different mechanics that may affect one Plane or the other, and can even reveal differences in level geometry. For each Plane, players can choose a specialization out of five different options, each of which corresponds to one of the ability schools used in the Saiyora.

__Available Callbacks:__ _On Plane Changed_  

__Modifiable Values:__ _None_  

__Restrictable Events:__ _Plane Swap_  

<a name="modern-plane"></a>
### 7.1 Modern Plane

The modern Plane represents a sci-fi world, where player loadouts focus around a primary weapon and a set of abilities that are mostly defensives, movement, and utility. Though there are damage abilities available in modern specializations, they are less fit for creating a cohesive toolkit to maximize output, and more aimed at being situational tools to handle specific situations, while damage primarily comes from shooting the specialization's weapon. Modern specializations for players include:  

- Chronomancer (Sky)  
  - Abilities based around manipulation of time, such as granting quick movements or reversing damage taken.  
- Mutilator (Fire)  
  - Abilities based around transformation of the player and enemies, resulting in crowd control, debuffs, and damage buffs, often trading health for utility or damage.  
- Telekinetic (Earth)  
  - Abilities based around control of gravity and force, allowing for displacement, mobility, and some burst damage options.  
- Illusionist (Water)  
  - Abilities based around changing perception, granting control over threat, aggro radius, and stealth.  
- Soldier (Military)  
  - Abilities based around technology and weaponry, sort of a jack-of-all-trades pool of abilities with mobility, healing, damage buffs, and crowd control.  

I have not yet decided on weapons for each specialization, but the idea is that each specialization has a unique weapon which deals damage and also brings some form of utility that is unique to the specialization. Out of the five remaining ability slots on the player's action bar, two are able to be picked from a pool of specialization-exclusive abilities that other specializations can not use, and three are able to be picked from a pool of abilities available to all modern specializations. The talent customization in this Plane's build thus comes from selection of the five non-weapon abilities on the player's bar. In addition, each specialization grants a passive buff called an Echo when the player is in the opposing Plane (so modern specializations grant a passive buff to the player while they are in the ancient Plane, and ancient specializations grant a passive buff to the player while they are in the modern Plane).  

<a name="ancient-plane"></a>
### 7.2 Ancient Plane

The ancient Plane represents the old world, thousands of years in the past, where player loadouts focus around a set of abilities that come together to create a damage "rotation" similar to how you would expect core abilities to work in something like an RPG game. There is some opportunity for utility, defensives, and mobility, but most abilities serve the purpose of dealing damage (or healing, in the case of one specialization). Ancient specializations for players include:  

- Stormbringer (Sky)  
  - This specialization is based around a resource called Ethereality that represents the strength of a player's anchor to the ancient Plane. At high ethereality, damage taken is reduced, and at low ethereality damage done is increased. The spec focuses around managing summoned tempests that constantly pull themselves back to the player and must be pushed away at enemies to deal damage and generate ethereality, as well as managing a conductivity debuff on enemies that enhances and is applied by various lightning-based attacks.  
- Pyromancer (Fire)  
  - This spec is based around managing damage over time effects called Embers, that gradually deal more damage over their duration. It excels in large-scale battles where a player can spread Embers to many targets, then force them to flare up into infernos that deal damage to nearby enemies. The player can also absorb high-powered Embers from enemies to increase damage from a variety of other fire-based attacks.  
- Shaman (Earth)  
  - This spec is actually more of a hybrid option, using manipulation of earth to create defenses for teammates in the form of large stone shields and path-blocking monoliths, while dealing damage primarily by piercing enemies with stone shards. The spec also has the option to sacrifice stone shards to create a sandstorm that protects allies and reduces enemy damage.  
- Spiritualist (Water)  
  - This is the only primary healing spec in the game at the moment. Spiritualists draw upon groundwater in an area to create soothing streams of healing, restorative wells of life, and torrential downpours to slow and damage enemies. It can also cleanse allies of damage over time effects and crowd controls, but can only play in one area for so long before exhausting the water supply there and needing to move.  
- Knight (Military)  
  - Knight is a melee specialization that focus on different combat stances. Each stance grants the Knight slight modifications to its abilities, resulting in gameplay that can alternate between frantically building toward a large burst of damage from impaling an enemy, parrying and knocking away enemies to survive, and cleaving enemies in a large area and causing huge amounts of threat. Currently, optimal play would result in this being similar to a tanking specialization in other games, due to a lot of threat generation.  

Talent customization in the ancient Plane is ability-based, where each ability in a specialization has two alternative versions that can be picked from, allowing different damage profile opportunities, additional utility, defensives, threat control, or mobility at the cost of damage. In addition, each specialization grants a passive buff called an Echo when the player is in the opposing Plane (so modern specializations grant a passive buff to the player while they are in the ancient Plane, and ancient specializations grant a passive buff to the player while they are in the modern Plane).  

<a name="plane-swapping"></a>
### 7.3 Plane Swapping

Currently, players can swap to the opposite Plane freely at the press of a button. Plane swapping is a restrictable event handled by the UPlaneComponent, so some mechanics can force players to stay in a specific Plane, but most of the time Plane swapping is a choice that players make based on availability of abilities in each Plane, encounter mechanics, incoming damage, and utility needs.  

The concept of "cross-plane" events exists for damage, healing, buff application, and threat actions, and usually implies a reduction in effectiveness due to the target and instigator of the event being in opposite Planes.  

<a name="plane-attunement"></a>
### 7.4 Plane Attunement

Plane attunement is a gameplay mechanic intended to encourage playing in one Plane or the other and not treating the two Planes as simply two action bars. Essentially, time spent in a Plane builds up a buff that increases player damage done and reduces player damage taken in that Plane. Swapping Planes causes the buff to slowly lose stacks until it is removed, and then a buff for the new Plane starts to stack. Ideally, this should promote gameplay where swapping Planes for a specific utility button, crowd control, or defensive, then swapping back, would be beneficial, but swapping Planes constantly to get all damaging abilities in both Planes on cooldown at all times would result in an overall loss of damage and survivability.

This is probably going to depend heavily on tuning, as ultimately many mechanics will encourage being in one Plane or the other, and dealing damage to enemies in opposite Planes is less effective, causing Plane swapping itself to be useful as a defensive or offensive boon. An alternative would be simply having Plane swapping be a cooldown of medium length, but that would close off the opportunity to create two builds that do have some synergy with each other.  

Plane attunement is not yet implemented.  

---

<a name="movement"></a>
## 8. Movement

In Saiyora, movement is handled through a USaiyoraMovementComponent, which is derived from UCharacterMovementComponent to add some interaction with crowd control, stats, and abilities. It also adds functionality for predicting specific movement effects and handling ability-sourced root motion. Epic's Character Movement Component is fairly complicated, and my movement system is built on top of it, so there are quite a lot of small implementation details that are necessary to prevent excessive server corrections, prevent cheating, allow smooth prediction, and support misprediction.  

__Available Callbacks:__ _None_  

__Modifiable Values:__ _None_  

__Restrictable Events:__ _Custom Movement_  

<a name="custom-moves"></a>
### 8.1 Custom Moves

Currently, Saiyora supports a handful of different movement types beyond the normal walking, jumping, and crouching build in to the Character Movement Component. These movement types are:  

- Root Motion  
  - This has its own section below.  
- Teleporting  
  - This moves an actor to a new location instantly, with options for stopping movement and setting a new rotation.  
- Launch  
  - Character Movement Component out of the box has a LaunchCharacter function, but it isn't predicted by default. I have implemented prediction and included options for custom force input and stopping vs adding to current momentum.  
  
Here is the flow for executing a custom move:  

- Check that the move can be performed.  
  - Is the move type valid?  
  - Is the source of the move valid?  
  - Does the owning actor have a Damage Handler, and if so, is the owning actor alive?  
  - If the move is not marked as Ignore Restrictions, is the owner rooted?  
  - If the move is not marked as Ignore Restrictions, are there any movement restrictions?  
- Check if the move either doesn't come from a UCombatAbility, is being executed by an actor that isn't the owning actor, or has a prediction ID of 0. Essentially, these are checking if this move should be involved in the prediction system at all. If any of the conditions are true, this move is NOT predicted.  
  - If any of these conditions are true:  
    - Check if the move is being performed on the server.  
    - Check if another custom move from the same source has already been executed this tick. This check is specifically to handle the fact that listen server players will call Predicted Tick and Server Tick back to back, and both will try to execute the same custom move.  
    - Check if the owning actor is locally controlled.  
      - If so, multicast the move execution using ExecuteCustomMove, which will cause all clients to perform it.  
      - If not, set a timer the length of the owning client's ping, RPC the owning client to "predictively" use the move, then after the timer, multicast the move execution to all clients using ExecuteCustomMoveNoOwner (which the owning client will ignore). This avoids server corrections by allowing the owning client to perform the move first. There is some potential to abuse this by falsifying ping to delay the application of moves, but there is an implemented max ping delay of .2 seconds to limit this.  
  - If all conditions were false (this move comes from an ability, is executed by an actor on itself, and has a valid prediction ID):  
    - If on the server:  
      - Check that a custom move with this prediction ID and tick number (from the source ability) has not already been executed. This is to prevent a race condition between an ability request RPC and the Character Movement Component's movement RPCs, which carry custom move data and are unreliable due to being sent on tick.  
      - Multicast to all clients to execute the move using ExecuteCustomMoveNoOwner (which the owning client will ignore).  
    - If on the owning client:  
      - Subscribe to the ability handler's On Ability Tick event with the OnCustomMoveCastPredicted function.  
      - Set up a pending custom move struct with the ability class, move parameters, prediction ID, and timestamp of prediction.  
      - When OnCustomMoveCastPredicted is triggered, unsubscribe from On Ability Tick (it was only necessary to allow the ability tick to complete and set any parameters for an ability request). Copy the target and origin parameters from the ability tick into the pending custom move, and mark the movement component's bWantsCustomMove flag as true.  
      - On the next tick, the component will call OnMovementUpdated as part of its normal movement ticking. In this function, the bWantsCustomMoveFlag being checked will call CustomMoveFromFlag, which locally will execute the custom move.
      - Also on that tick, the Character Movement Component will create a new Saved Move, which contains the pending custom move and the flag indicating that a custom move should be executed. This move is used for replaying moves when corrections happen on the client, so there must be a record of the custom move here.  
      - The Character Movement Component will finally fill out its Network Move Data for this tick, which is the data that is actually sent to the server in movement RPCs every tick. Here, an ability request mirroring the one made in the ability handler for the ability usage that caused the custom move will be added to the network move data.  
      - On the server, this networked move data will be copied back into the Character Movement Component, and the flag for a custom move will once again be checked in OnMovementUpdated, which will once again call CustomMoveFromFlag. This function will now (after checking against the race condition mentioned above for duplicate movement) call UseAbilityFromPredictedMovement in the ability component. At this point, the ability system will call its normal server ability usage logic, and eventually wind up executing the custom move either immediately, or when the correct tick of the ability happens if the RPC came too soon.  
      - When the movement is executed on the server, it will also be multicast using ExecuteCustomMoveNoOwner (and the owning client will ignore it).  
      
Most of this complexity comes from two sources:  
- The already massive complexity of the custom move system built into the Character Movement Component.  
- The race condition caused by unreliable RPCs from the Character Movement Component not being guaranteed to happen before or after the reliable RPC from the Ability Handler.  

Both of these are handled with a multitude of checks preventing duplication of movement execution from the same ability tick and integration of the ability request struct into the structs used by the Character Movement Component such as saved moves and network move data.  

<a name="root-motion"></a>
### 8.2 Root Motion

