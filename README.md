# Saiyora Technical Overview

---

## Combat

_A quick note about Epic's Gameplay Ability System:_
_I did experiment with using Unreal Engine's Gameplay Ability System plugin for this project, but ultimately decided that creating my own ability system would give me more control over when and how prediction and lag compensation occurred, and allowed me to build things such as multiple-charge cooldowns in from day 1 without having to rework GAS's built-in cooldown functionality. I also didn't like the structure of GameplayEffects as non-instanced attribute modifiers and GameplayAbilities as any instanced functionality, as conceptually there were many "buffs" in my initial outline of specializations that would fall under the GameplayAbility umbrella. I opted instead for a clearer distinction of Ability vs Buff in my system, at the cost of instancing, replicating, and destroying buffs._

---

### Abilities

Abilities are handled by the UAbilityComponent, which is a universal actor component that can be applied to players or NPCs to give them functionality to add, remove, and use objects derived from UCombatAbility, the generic ability/spell class in this game. For players, UAbilityComponent also handles prediction and reconciliation of the global cooldown timer, individual cooldowns, cast bars, resource costs, and any predicted parameters necessary to fully predict most ability functionality, such as aim targets and camera location.  

##### Ability Usage Flow

Ability usage is fairly complicated, especially when looking at prediction and replication. A basic overview from the perspective of a server-controlled actor (either an NPC or a player acting as a listen server) using an ability is as follows:
- Request ability use by TSubclassOf<UCombatAbility>.
- Check if the actor is locally controlled and the ability subclass is valid.
- Check active abilities to find one of the requested class.
- Check if the ability is usable:
    - Is the ability valid, initialized, and not deactivated?
    - Is the caster alive, or is the ability usable while dead?
    - Does the ability have sufficient charges?
    - Are the ability's resource costs met?
    - Are there any custom cast restrictions?
    - Are any of the ability's tags restricted (including the ability class tag)?
    - Is the global cooldown active, or is the ability off-global?
    - Is another cast in progress?
- Start the global cooldown, if the ability is on-global.
- Commit the ability's charge cost and start the ability's cooldown if any charges are spent.
- Commit any resource costs of the ability.
- In the case of an instant ability: Execute the predicted tick and server tick functionality of the ability.
- In the case of a channeled ability: Start a cast bar, and execute the predicted tick and server tick functionality if an initial tick is needed.
- If an ability tick was executed, multicast to execute the simulated tick on other machines.

For players that are not a listen server, things are more complicated, as there is a lot of prediction going on. Here is an overview of a predicted ability use by a non-server player:

ON CLIENT:
- Request ability use by TSubclassOf<UCombatAbility>.
- Check if the actor is locally controlled and the ability subclass is valid.
- Check active abilities to find one of the requested class.
- Check if the ability is usable:
    - Is the ability valid, initialized, and not deactivated?
    - Is the caster alive, or is the ability usable while dead?
    - Does the ability have sufficient charges?
    - Are the ability's resource costs met?
    - Are there any custom cast restrictions?
    - Are any of the ability's tags restricted (including the ability class tag)?
    - Is the global cooldown active, or is the ability off-global?
    - Is another cast in progress?
- Generate a new Prediction ID.
- Predictively start a global cooldown, if the ability is on-global.
- Predictively commit the ability's charge cost, and start an indefinite-length cooldown if charges were spent and the ability isn't already on cooldown.
- Predictively commit any resource costs of the ability.
- In the case of an instant ability: Execute the predicted tick functionality of the ability.
- In the case of a channeled ability: Start an indefinite-length cast bar, and execute predicted tick functionality if an initial tick is needed.
- Send ability request to the server.

ON SERVER:
- Check request for valid prediction ID.
- Perform all of the steps taken on the client, but with actual calculated values for cooldown length and cast bar length, and executing the server tick functionality instead of predicted tick functionality.
- If a tick was performed, multicast to execute simulated tick functionality on other clients.
- Notify the client of the result of the ability request.

ON CLIENT:
- Update resource cost predictions from server result (this does nothing if the server has already replicated down new resource values with the same prediction ID).
- Update ability charge cost from server result (this does not set the cooldown timer to the correct length, just corrects any mistakes in calculating charge expenditure).
- Update global cooldown from server result (this DOES adjust the GCD timer if necessary, since GCD length is predicted).

#### Ability Usage Conditions

Abilities need to be initialized on the server and owning client, and are not usable until this is done. This initialization involves making sure the ability has a valid reference to it's owning Ability Component, valid max charges, charges per cooldown, and charge cost, current charges set to max, resource costs set up, valid references to all resources that are included in the ability's costs, any custom cast restrictions that need to be set, any Blueprint initialization handled, and castable status updated properly. Most of this only takes place on the server, and the client can just call the custom cast restrictions, Blueprint initialization, and castable status steps once a valid Ability Component reference has been replicated down.

Abilities have the option to be castable while dead, where they will ignore their owner's life status (if the owning actor even has a Damage Handler). In the event that the owner does not have a Damage Handler, this check is skipped entirely.

A number of ability-specific restrictions on usage are encapsulated in the Castable enum, which holds a reason that an ability is not castable at any given time, or None if the ability is castable. It is constantly updated by multiple different checks for the purpose of being displayable on the UI for players. These checks include:

- Charge cost check, updated any time the charge cost or current charges of the ability are updated.
- Resource cost check, updated any time any of the resources the ability depends on change values (or are removed).
- Custom cast restrictions check, updated any time a custom cast restriction is activated or deactivated by the ability. These are for conditions that the ability imposes on itself, for example "Can only cast while moving," "Must have a debuff active on an enemy to cast," etc.
- Tag restrictions check, updated any time a restricted tag is added or removed from the ability. These are restrictions applied by sources other than the ability itself, including a generic restriction on an ability class (and all its derived classes) and more specific restrictions that are checked against an ability's tags.

Abilities marked as "On Global Cooldown" can not be activated during a global cooldown.

Abilities can not be activated during a channel of an ability. For players, there is some functionality setup inside the Player Character class (where ability input is initially handled) to automatically try and cancel a channel in progress to use a new ability if the channel isn't about to end (more on that in the Queueing section).


##### Global Cooldown

_Modifiable Values: Global Cooldown Length_

The global cooldown (GCD) is a concept commonly used in non-shooter games. Specifically, MMOs like World of Warcraft and Final Fantasy use GCDs as a way to set the pace of combat, causing a short period after using most abilities where other abilities can not be used. Some abilities designated as "off-GCD" can ignore the GCD and not trigger its timer, but in the games I used as examples, the vast majority of core abilities trigger a GCD anywhere from 1 to 3 seconds. 

In Saiyora's combat system, most core damage and healing abilities trigger a GCD in the range of 1 to 1.5 seconds, to create a similar feel to combat systems you would find in RPGs where damage throughput comes mostly from selecting the correct ability at the right time rather than from spamming abilities as fast as possible. The most notable exceptions to this are weapons, which do NOT trigger a GCD (or have a cooldown), and instead manage fire rate separately (which is detailed more in the Weapons section later on). Additionally, most defensive, mobility, and utility skills are off-GCD, allowing them to be used fluidly and reactively as needed.

GCDs are client predicted in the Ability Component, including their length (which differs from the approach used to predict cooldowns and cast bars later). When a client activates an ability that triggers a GCD, they will calculate the GCD length locally and immediately start the GCD timer. In the case of misprediction, the local GCD timer is cancelled if it still active and still on the GCD started by the mispredicted ability use. In the case of success, the server will send the client its calculated GCD length as part of the response, in which case the client can use its own predicted ability start time to determine a new time for the GCD to end. 

There is potential for issues if the ping of the client drops in between ability uses, as a request for using a new ability might arrive before the previous GCD has ended on the server, despite the client locally seeing his predicted GCD complete. For this reason, the server sends the client the correct calculated GCD length, but actually sets its own GCD timer to a reduced value, based on a universal multiplier of the requesting client's ping (currently set at 20%), so that it would be exceedingly rare for a client's ping to fluctuate enough naturally to cause their next ability request to arrive before the previous GCD had finished on the server (without cheating or completely mispredicting). There is some potential for cheating here, as clients could send new ability requests up to 20% earlier by bypassing their local GCD timer, and the server would accept this as valid. One way to combat this would be to do some internal bookkeeping of GCD discrepancies over time, and if clients are consistently sending new ability use requests within that 20% window, disconnect them. However, I have not implemented this functionality right now.

##### Cooldowns and Charges

_Modifiable Values: Charge Cost, Cooldown Length, Charges per Cooldown_

Cooldowns in Saiyora are based on a charge system. This means every ability can be used a certain amount of times before running out of charges and needing to wait for the ability's cooldown timer to finish and refund some or all of those uses. A basic ability would have a single charge, and its cooldown would restore that single charge, which would mirror the functionality most commonly seen in RPGs or in Epic's own Gameplay Ability System. However, the charge system gives flexibility to design abilities that can also be fired multiple times before needing to cooldown, and even the ability for a cooldown to restore multiple charges to an ability, creating some resource management and "dump" windows for abilities where it is encouraged to use multiple charges in quick succession to avoid wasting refunded charges from the ability's cooldown.

Prediction of cooldowns is a little less robust than prediction of GCDs, as they are typically longer in length than GCD, and thus it is unlikely that an ability would go on cooldown and then have that cooldown complete within a window the length of the client's ping. They also can "roll over" in the case of multiple charge abilities that don't get all their charges back in one cooldown cycle, in which case my normal prediction ID system would need to be reworked to account for new cooldowns as a new prediction, despite no ability use triggering them. As such, charge expenditure is predicted when an ability is used, and the cooldown of the ability will "start" locally, however no timer will be displayed and length will not be calculated.

The server keeps track of the authoritative cooldown state, calculating charge cost and cooldown length when using an ability, then setting a replicated cooldown state struct in the ability itself for the owning client to use. On replication of the server's cooldown state, the client will then discard any outdated predictions (identified by prediction ID, which the cooldown state contains) and accept the server's cooldown progress. If there are remaining predictions, the client will then recalculate its predicted charges and start a new indefinite-length cooldown if needed. In the case of misprediction, similar steps are taken, with only the mispredicted charge expenditure being thrown out and then the cooldown progress being recalculated.

Currently on my list of TODOs: Adding in the same lag compensation window to cooldowns triggered by an ability use (NOT cooldowns triggered by "rolling over" at the end of a previous cooldown) that GCD uses, to prevent players with higher ping from being disadvantaged with longer cooldowns on abilities.

##### Casting

_Modifiable Values: Cast Length_

There are two types of abilities in Saiyora: instant and channeled. Instant abilities happen within a single frame, while channeled abilities happen over time, in a set number of "ticks" (note this is not Unreal Engine's game tick, but a term adopted from MMOs that represents an instance of an ability activation within a set triggered by a single cast) that are equally spaced throughout the ability's duration. Channeled abilities can either have an initial tick or not, and then have at least 1 non-initial tick where the ability's functionality is triggered, the last of which occurs at the end of the cast duration.

Since abilities are always instanced in Saiyora, they can keep internal state between ticks and between casts, and are also passed the tick number each time they activate (for instant casts this is always 0) to allow abilities that perform different actions throughout their duration. Channeled abilities will cause their caster to display a cast bar in the UI, showing progress through the cast.

Channeled abilities also have the ability to be cancelled and interrupted. Interrupting will have its own dedicated section later. Cancelling a channeled ability simply ends it and prevents any further ticks from happening. Both cancelling and interrupting can trigger behavior on the ability if needed through seperate OnInterrupted and OnCancelled functions.

Prediction of casts is handled similarly to cooldowns, where the predicting client will instantly trigger the initial tick (if the ability has one), and display an empty cast bar, but will not calculate the length of the cast or trigger any subsequent ticks until the server has responded with the correct cast length or notice of misprediction. Misprediction simply results in the immediate end of the cast (this is not considered a cancellation and will not trigger behavior), while confirmation of the cast length allows the client to use his predicted activation time to recreate the cast bar, trigger any ability ticks that may have been missed in the time between predicting and receiving confirmation of the ability usage, and set up a timer for the next tick. Each tick is predicted separately, with the client sending any necessary data at his own local tick intervals, and the server simply holding that data then triggering the tick at the right time (if it arrives before the server ticks), or marking the tick as lacking data and triggering it when client data arrives (if it arrives after the server ticks). There is a time limit on how far out of sync the server will execute a predicted tick to prevent the client from sending ticks at incorrect times and potentially causing issues. Currently, this limit is 1 tick, so the server will ignore a tick entirely if the next tick of the ability happens on the server without receiving prediction data. This may need to be adjusted if there is a need for a lot of fast-ticking abilities used by players (as NPCs don't need to worry about prediction).



