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

Combat actors appear differently to the local player depending on their faction and plane relationship to that player. Typically, combat actors that are xplane from a player will appear to be shaded in purple, while same plane actors have their normal shading. Actors that are of the Friendly faction are outlined in green, with Neutral actors in yellow and Enemy actors in red.

This is accomplished using a custom post-process material applied to the entire world. The UCombatStatusComponent keeps track of the local player's current faction and plane, as well as it's own faction and plane, and updates the custom stencil value of its owner's primitive components to represent the correct faction and plane relationships.  

The custom stencil value provided in Unreal has a range of 0-255 (8 bytes). For the outline material to display when an actor is overlapping another actor, each actor must have a unique stencil value for the post-process material to be able to handle edge detection. That led to this distribution of stencil values:

> 0 = Enemy actors, same plane (default value if all values from 1-49 are assigned)  
> 1-49 = Enemy actors, same plane  
> 50 = Enemy actors, xplane (default value if all values from 51-99 are assigned)  
> 51-99 = Enemy actors, xplane  
> 100 = Neutral actors, same plane (default value if all values from 101-149 are assigned)  
> 101-149 = Neutral actors, same plane  
> 150 = Neutral actors, xplane (default value if all values from 151-199 are assigned)  
> 151-199 = Neutral actors, xplane   
> 200 = Local player, or target with no shading/outline  
> 201-227 = Friendly players, same plane  
> 228-255 = Friendly players, xplane  

There is a static map of render values to UCombatStatusComponents that keeps track of which stencil values are already in use on the local machine. When a local player swaps planes, or a UCombatStatusComponent swaps planes, this value is re-evaluated to find a new appropriate value, and the old value is opened up. Due to the limited number of values the custom stencil supports, there is a possible situation where more than 49 actors of the same faction/plane relationship to the local player could be active at once. In this case, additional actors receive a default value (the lowest value in their appropriate range) that is shared with other actors in the same situation. These actors will render correctly in all situations except when they overlap each other, in which case their outlines will not show up on any pixel where they overlap (as they have the same custom stencil value). This issue does not apply to players, as currently there is no situation in which more than 4 players are in the same game at a time, let alone 27. In the future, if Friendly NPCs become common, this may need to be refactored to use the Neutral range for Friendly instead, depending on which is more common.

## Combat Collision  

Similar to how plane can affect collision with level geometry, faction can affect the collision of projectiles, traces, and hitboxes. Projectiles can be dynamically set to interact with hitboxes of only players or NPCs, allowing functionality like shooting through friendly players, in addition to ignoring or colliding with xplane level geometry. Tracing can use any of a large number of profiles for every combination of plane collision and hitbox collision, with different options for hitting or overlapping hitboxes to allow multi-traces.  

**[⬆ Back to Top](#top)**
