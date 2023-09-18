<a name="top"></a>
**[Back to Root](/README.md)**

# Abilities

The ability system in Saiyora handles reusable actions that actors can activate during gameplay. These actions support cooldowns, charges, global cooldowns, cast times, channel ticks, restrictions, resource costs, and runtime modification of all of the aforementioned aspects via stats and buffs. Abilities are designed with replication in mind, which means they support client prediction and are server authoritative. Ability usage is handled by the UAbilityComponent (or a child class).

In addition to basic information like a name, icon, and description, abilities also have a set of GameplayTags describing them, which can be used to restrict or find certain abilities an actor has active, and a Plane setting that can be used to assign abilities to the correct player hotbar or determine in what Plane an NPC can use a certain ability.

Abilities can be added and removed from actors at runtime on the server. This is most useful for NPCs who gain or lose abilities during combat, such as bosses with multiple phases. For players, there is a restriction in place that prevents abilities from being added or removed during dungeon gameplay, but this is specific to the blueprint player character class used for dungeons, and not part of the ability system itself.

## Ability Functionality

Ability functionality is comprised of three different events, all exposed to Blueprints.
- OnPredictedTick: This fires on the owning client immediately upon activating an ability, and is meant to be used for instant feedback such as particle effects and sounds, as well as gathering aim and target information for abilities that need these things.
- OnServerTick: This fires on the server, once the ability's activation has been verified. This is where actual gameplay effects happen, such as applying buffs or dealing damage. The server has access to any parameters gathered during the predicted tick for use in things like rewinding hitboxes or using client movement input.
- OnSimulatedTick: This fires on all clients via a multicast RPC after the server tick. Clients once again have access to any parameters gathered by the server or owning client if needed to recreate accurate effects such as hit effects.

Notably, these events can fire on the same client multiple times. For example, a listen server would fire all three events in rapid succession, while an autonomous proxy would fire both the predicted and simulated ticks (with a delay between them while the server verifies). This means it is important to check net role in some cases to not double-fire certain effects.

> An easy example would be firing a weapon. On the owning client, the Predicted Tick event would likely create some kind of muzzle flash particle effect, play a sound, and record the player's camera location, aim vector, and whatever the shot hit as parameters to be passed to the server. On the server, the Server Tick event would verify through hitbox rewinding that the target was actually hit when using the provided aim vector at the time the client would have shot, and then apply damage to that target. The Simulated Tick event would play the muzzle flash and sound for non-owning clients, and play some kind of hit animation on the hit target for all clients. Damage numbers and hit sounds would likely be handled by damage event replication for the owning client, rather than by the ability.

## Cooldowns, Charges, and the GCD

Abilities support cooldowns in the form of a charge system. The most common system for ability cooldowns is an ability that only be used once every few seconds, and in Saiyora's system this is just implemented as an ability with only a single charge. However, an ability's max charges, charges per cooldown, charges per cast, and cooldown length are all editable values (and can optionally be modified at runtime as well). 
> One example of a multi-charge cooldown is the Untether ability. This ability has two charges, and restores both of those charges upon its cooldown finishing. This means that using the first charge starts the cooldown, and the second charge is still available to be used at any time before the cooldown finishes. Whether one charge or both are used, the ability will return to two charges at the end of the cooldown. This allows Untether to have a "prime" and "execute" mechanic tied to the first and second charges respectively, where the first charge lifts enemies into the air and the second charge slams any lifted enemies down.

Cooldowns feature some client prediction. Charge usage is predicted to prevent a client from erroneously spamming an ability and having to be corrected. Clients will see the ability charges go down, and see the cooldown start, but the cooldown will not tick down until the server has verified the cooldown time and replicated it back to the client. This is to prevent having to calculate cooldown times on the client, which can lead to mispredictions due to buffs or stats modifying the cooldown on the server but not yet being replicated to the client. In practice, ability cooldowns are never short enough where the cooldown would finish before the server's state replicates back to the client, barring absurdly bad net conditions, in which case the client would just automatically finish the cooldown upon receiving the server's state.

The global cooldown (GCD) is a shared cooldown across all abilities considered "On GCD." When any ability on the GCD is used, all abilities on GCD go on a short cooldown that essentially just prevents pressing multiple abilities at once that aren't designed to be used at the same time. Most non-weapon attacks are on GCD, while things like movement or any ability that expects a fast reaction time to be useful are typically off GCD (and don't trigger or abide by the GCD at all). The GCD promotes more thoughtful ability usage in scenarios where multiple abilities that deal damage are available at the same time, and choosing the correct order to activate them results in higher overall damage. For this reason, it is a very common system in RPG games such as World of Warcraft, where skill expression in dealing damage is often about prioritizing ability usage correctly.

## Casting and Ticking

Abilities have two cast types: Instant and Channeled. 
- Instant abilities do what they say, activating instantly and performing all of their effects in one frame. Instant abilities can optionally be marked "Castable While Casting," which means they can be activated during a channel of another ability without cancelling it.
- Channeled abilities happen over a duration, using a set number of "ticks" to perform their functionality. Channeled abilities can be used to emulate traditional RPG spell casts (where the cast only does something if the channel is completed), typical channeled abilities (the ability does something on an interval until the cast ends), or persistent effects (activating some functionality on the initial tick of the channel and then ending that functionality when the cast ends or is cancelled). Each tick of a channeled ability calls the same Predicted, Server, and Simulated events that an instant ability does. Channeled abilities are marked as "Castable While Moving" by default, but this flag can be turned off which will cause any movement to cancel the channel early.

Both types of ability expend their resource and charge costs immediately by default, though for a more traditional spell cast type of effect the charge can be manually removed in the final tick of the cast instead. Currently, manually adjusting charges is not client predicted (whereas the normal charge cost is), but I intend to implement this functionality to support more traditional cast style abilities if needed.

Cast time is currently client predicted. Since ticks are spread evenly throughout a cast, mispredicting cast time due to lag on modifiers to said cast time can cause some ticks to be fired early or skipped over when receiving the authoritative cast time on the owning client. This is handled in the correction logic, where ticks fired early by the client will await their correct time to be fired on the server, and ticks missed by the client will be immediately fired and sent to the server upon correction. This means that any lag should have no adverse effects on the number of channel ticks an ability will have per cast.

## Restrictions

Ability usage is a restrictable event. Due to how much of ability usage is client predicted, this means restrictions to usability need to be available to the owning client. To solve this, each ability has a simple "Castable" flag that is constantly updated on both the server and client. This flag takes into account charge cost, global cooldown status, resource costs, custom cast restrictions, and tag restrictions.

Custom cast restrictions are restrictions an ability places on itself to prevent casting in certain scenarios. They are represented as GameplayTags that can be added and removed when criteria is met at runtime.

> For example, the spell Inferno is only usable when at least one enemy has a Wildfire buff with 3 or more stacks active. To facilitate this, the Inferno spell tracks active Wildfire buffs applied by its owner, and when at least one has 3 or more stacks, it removes the restriction GameplayTag "HighWildfire" from itself. If no active Wildfire has 3 or more stacks, it readds this tag.

Tag restrictions are restrictions placed on the Ability Component, preventing abilities with matching tags from being used. Since these restrictions are only added on the server, abilities have a replicated bTagsRestricted flag that tracks whether any of their tags are currently restricted and allows the owning client to keep their Castable flag updated.

Abilities can also be restricted by class. When restricting an ability class, the Ability Component simply adds the AbilityRestriction.Class tag to the ability's restricted tags container, which will work with the bTagsRestricted flag like any other tag restriction.

## Cancels and Interrupts

Cancelling and interrupting are two different ways of stopping a channeled ability use before its completion. 
- Cancelling is typically a harmless way to stop an ability usage, and can be client predicted. The most common use case for cancelling is that a player presses the button to use another ability early, but swapping planes during a cast of a plane-restricted ability or pressing escape are cancels as well.
- Interrupting is the intentional stop of a cast by another actor (though it is possible for an actor to interrupt themselves). This is server-only and is a restrictable event. Dying during a cast that is not marked as "Castable While Dead", moving during a cast that is not marked "Castable While Moving," or being crowd controlled with a crowd control in the casting ability's restricted crowd control list are also considered interrupts.

Both cancelling and interrupting have Blueprint-exposed events inside the ability, as well as delegates for successful execution in the Ability Component. The Blueprint events mimic the Predicted, Server, and Simulated Tick events, with OnPredictedCancel, OnServerCancel, OnSimulatedCancel, OnServerInterrupt, and OnSimulatedInterrupt available for abilities that want to tie functionality to these circumstances.

## Weapons

Weapons are a subclass of UCombatAbility that do things a bit differently to support fast consistent fire rates that weapons need, as well as automatically set up a weapon object attached to the actor that can respond to fire and reload events. Most of the actual firing logic is in a UFireWeapon subclass (which is itself just a UCombatAbility), that handles setting up the AWeapon subclass to be used as the weapon's visual representation, a UStopFiring ability to handle stopping simulated effects, and optionally the UReloadWeapon subclass that handles reload logic for that weapon. Adding a UFireWeapon-derived class will also automatically add the associated reload ability. Fire Weapon, Stop Firing, and Reload Weapon also have specific tags so that players can assign them to the correct keybinds/hotbar slots upon learning them.

Weapons do not use the charge system or have a typical cooldown. Instead, they use a custom cast restriction with the AbilityRestriction.FireRate tag to determine how often they can fire. The client handles adding and removing this tag between every shot, while the server simply verifies how many shots are fired over a given time span. Waiting on the server to add and remove the tag would cause mispredictions extremely commonly in fast-firing weapons under any sort of latency, so the verification is more lenient to allow for good gameplay under lag.

Visually, fast fire rates will not appear consistent if all of the effects are tied to the Simulated Tick RPC from the UFireWeapon ability. Instead, firing the weapon will cause the AWeapon to start firing repeatedly on an interval equal to the fire rate until a StopFiring ability is received. AWeapon's effects are strictly visual, so the actual gameplay effects such as damage from the weapon will still be handled in the Server Tick logic of Fire Weapon. Stop Firing is automatically called when Fire Weapon stops being activated (this is handled by the player character class right now). For weapons that are not automatic, the weapon functions as you would expect, with each individual shot causing visual effects tied to the Simulated Tick event.

Reloading is just another ability whose functionality is usually just to restore the Ammo resource, which is universal to all weapons, but theoretically can do anything necessary. Weapons also support optional auto-reloading when in another plane for a long period of time, but auto-reload can not have any additional functionality (it just restores ammo to max upon triggering).

**[⬆ Back to Top](#top)**
