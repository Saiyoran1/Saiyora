<a name="top"></a>
**[Back to Root](/README.md)**

# Combat Status

The UCombatStatusComponent encapsulates two important concepts for determining how actors interact with each other: Plane and Faction.

Plane represents which "version" of the world the actor currently inhabits. These options are the Ancient plane, Modern plane, both planes, or neither plane. Actors may also choose "None," which is an option that essentially removes any plane-based functionality from the actor. The actor's plane relationship (usually described as "xplane" or "same plane") to other actors determines its visuals, collision, and other gameplay interactions. Actors can change planes during gameplay through a handful of different abilities and mechanics.

Faction represents the actor's hostility toward other actors. Generally, players are considered "Friendly" faction, and NPCs can either be "Neutral" or "Enemy" faction. For actors that choose "None", this is usually treated as neutral in determining actor faction relationships. There is not currently a way to change factions during gameplay, but it is something I'd like to implement.

## Level Geometry Planes

Non-combat actors in a level can change their rendering and collision via the ULevelGeoPlaneComponent, which adjusts the materials on the actor's mesh components based on their plane relationship to the local player. ULevelGeoPlaneComponents have a default plane set in which their materials are displayed normally. If the local player is xplane to this default plane, then these materials are swapped out for an assigned "xplane material," which is usually something translucent to indicate that the local player can't interact with this actor normally. Currently, the default for this material is the Unreal Engine glass material.

Collision can also change based on plane. Actors using WorldDynamic collision by default that have a ULevelGeoPlaneComponent will have their collision object type swapped to WorldModern or WorldAncient (or remain WorldDynamic) at BeginPlay. Combat actors, projectiles, and traces then use profiles set to ignore level geometry of specific object types. Generally, actors that don't interact with the plane system keep their WorldDynamic object type, which is set to block in all relevant profiles. 

## Combat Actor Rendering  

Combat actors appear differently to the local player depending on their faction and plane relationship to that player. Typically, combat actors that are xplane from a player will appear to be shaded in purple, while same plane actors have their normal shading. Actors that are of the Friendly faction are outlined in green, with Enemy and Neutral actors in red.

This is accomplished using a custom post-process material applied to the entire world. The UCombatStatusComponent keeps track of the local player's current faction and plane, as well as it's own faction and plane, and updates the custom stencil value of its owner's primitive components to represent the correct faction and plane relationships.  

The custom stencil value provided in Unreal has a range of 0-255 (8 bytes). For the outline material to display when an actor is overlapping another actor, each actor must have a unique stencil value for the post-process material to be able to handle edge detection. That led to this distribution of stencil values:

> 200 = Local Player (no outline, no xplane shading)  
> 151-199 = Friendly actors, same plane (green outline, no xplane shading)  
> 150 = Friendly actors, same plane (default value for cases where every value from 151-199 is already assigned)  
> 101-149 = Friendly actors, xplane (green outline, xplane shading)  
> 100 = Friendly actors, xplane (default value for cases where every value from 101-149 is already assigned)  
> 51-99 = Enemy/Neutral actors, same plane (red outline, no xplane shading)  
> 50 = Enemy/Neutral actors, same plane (default value for cases where every value from 51-99 is already assigned)  
> 1-49 = Enemy/Neutral actors, xplane (red outline, xplane shading)  
> 0 = Enemy/Neutral actors, xplane (default value for cases where every value from 1-49 is already assigned)  

There is a static map of render values to UCombatStatusComponents that keeps track of which stencil values are already in use on the local machine. When a local player swaps planes, or a UCombatStatusComponent swaps planes, this value is re-evaluated to find a new appropriate value, and the old value is opened up. Due to the limited number of values the custom stencil supports, there is a possible situation where more than 49 actors of the same faction/plane relationship to the local player could be active at once. In this case, additional actors receive a default value (the lowest value in their appropriate range) that is shared with other actors in the same situation. These actors will render correctly in all situations except when they overlap each other, in which case their outlines will not show up on any pixel where they overlap (as they have the same custom stencil value). I am currently working on solving this issue, and also finding a way to allow yellow outlines for Neutral actors to distinguish them from Enemy actors.  

## Combat Collision  

There are three main types of collision that are relevant to combat. The first is pawn movement, which collides with level geometry and can be plane-dependent or not. The second is projectile collision, which has two separate categories: one for collision with geometry, which can be plane-dependent or not, and one for hitbox collision, which can be faction-dependent or not. The third is tracing, which can be affected by both plane and faction if necessary.  

All of these collisions are handled with a large number of collision profiles in the default collision settings of the engine covering every permutation necessary. Level geometry can use WorldDynamic, WorldModern, or WorldAncient, which are determined by the ULevelGeoPlaneComponent. Pawns can use Pawn, PawnModern, or PawnAncient, which are determined by the UCombatStatusComponent. Projectiles use ProjectileCollisionAll, ProjectileCollisionModern, or ProjectileCollisionAncient for their level geometry collision, and ProjectileHitboxAll, ProjectileHitboxPlayers, or ProjectileHitboxNPCs profiles to determine what hitboxes they hit, both of which are determined during initialization of projectiles. Tracing uses any of the many "CT_..." profiles to pick a combination of planes and factions they interact with, and whether they overlap hitboxes or are blocked by hitboxes.

**[⬆ Back to Top](#top)**