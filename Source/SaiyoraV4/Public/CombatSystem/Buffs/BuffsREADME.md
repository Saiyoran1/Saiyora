<a name="top"></a>
**[Back to Root](/README.md)**

# Buffs

The UBuffHandler component handles application and receiving of buffs, which are instanced as replicated UBuff objects. Buffs represent any change to combat state that happens for a duration of time, including modification to stats, restriction of events, periodic events like damage or healing over time, or threat actions. Buff application and removal are not client-predicted, and happen only on the server.

## Buff Application

When applying a buff, there are a few parameters passed in, including a Duplicate Override flag, a Stack Override type and value, a Refresh Override type and value, an Ignore Restrictions flag, and an array of Buff Parameters. The override flags are a way to bypass the default behavior of a buff class during gameplay when applying multiple buffs of the same class from the same actor.

- Duplication is a binary behavior, where an application of a buff either creates a new instance each time it is applied with its own parameters, stacks, and duration, or whether it instead just modifies an already existing instance of the same class.
- Stacking affects non-duplicating buffs by changing the value of an existing buff instance, usually by multiplying its effectiveness in some capacity or working toward some stack threshold that triggers new behavior. Stacking can be Additive or Overriding, and the amount of stacks to add or override with can also be passed in to bypass the buff's default stacking behavior.
- Refreshing affects non-duplicating buffs by changing the remaining duration of an existing buff instance. This also features an Additive or Overriding option to either extend the duration by an amount or set the duration to a new value, bypassing the buff's default refreshing behavior.

> As an example, you could create a buff that by default has a duration of 10 seconds, is refreshable with a default 10 second additive refresh, and is stackable with a default 1 additive stack and 1 initial stack, with a max stack count of 5 and a max duration of 15 seconds.
> 
> Applying this buff the first time would create an instance with 1 stack and a 10 second duration. 
> 
> Applying it again immediately would bump the stacks to 2 and the duration to 15 (the maximum).
> 
> Applying the buff a third time, but with a 3 stack additive override and a 5 second absolute override would result in a buff with 5 stacks and a 5 second duration.

Buff application is a restrictable event, using a buff application event struct as context for determining whether a buff can be applied. Additionally, buffs save off whether they were applied with the Ignore Restrictions flag, and will continue to ignore new restrictions that would otherwise remove them later in their lifetime. Finally, there is also a flag in the buff handler to never receive buffs, for actors that want to apply buffs to others but never be a viable buff target themselves.

## Buff Functions

Buff functions are reusable behaviors that buffs can instantiate and apply with custom parameters, for the purpose of avoiding writing common buff functionality in every buff that needs different values. Examples of buff functions that I have already implemented include:

- Damage Over Time
- Healing Over Time
- Stat Modification
- Crowd Control
- Event Restrictions
- Modifier Application
- Threat Actions

Each buff function is its own class derived from UBuffFunction, which contains callbacks for an owning buff's OnApplied, OnStacked, OnRefreshed, and OnRemoved events, as well as a SetupBuffFunction and CleanupBuffFunction for pre-initialization setup and post-removal clean up. Each buff function class declares a static BlueprintCallable function that creates an instance of the buff function, registers it with the owning buff, and passes any necessary parameters to a class-specific SetVariables function. From there, buff functions simply override the behavior callbacks that are triggered by the owning buff to do whatever it is they need. Buff functions are instantiated and setup in the owning buff's SetupCommonBuffFunctions call.

Because of the diverse nature of required parameters for buff functions, an alternate approach where buff functions are initialized from structs of buff function subclasses and parameters wasn't really feasible without creating an extremely generic parameter class, and even then, a lot of buff functions take function delegates as parameters (to enable conditional modifiers and event restrictions), which aren't supported currently as Blueprint variables in the editor beyond some limited functionality.

**[⬆ Back to Top](#top)**
