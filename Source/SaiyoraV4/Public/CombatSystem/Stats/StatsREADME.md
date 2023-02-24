<a name="top"></a>
**[Back to Root](/README.md)**

# Stats

Stats are modifiers to values used in combat that can be changed at runtime by buffs. These include modifiers to things like damage done, damage taken, max health, max walk speed, combat detection radius, or anything else necessary for creating unique mechanics. These are notably not replacements for values handled in other components like current health, max health, or max walk speed, and just act as modifiers that those components can read and adjust their values with. 

Stats are managed and replicated by the UStatHandler, and are represented as a FFastArraySerializedItem-inherited struct that contains an FModifiableFloat and a GameplayTag. Using fast arrays allows for per-item callbacks, which is useful to trigger client-side behavior when stat changes are replicated. Each stat an actor has must use a unique GameplayTag that begins with "Stat."

Stats can only be modified on the server, which can cause some issues specifically with stats that affect movement. Stat modifier application and removal are normally handled by a buff function I have already written to handle stacking behavior and automatic modification on buff application and removal. However, these functions can also be manually handled on the server by other actors if needed.

## Stat Initialization

Stats are initialized for an actor in one of two ways:
- Using a Stat Template, which is a UDataTable asset whose rows are comprised of FStatInitInfo structs, which contain info about the default value, optional max and min values, whether the value can be modified at all, and the stat's tag.
- Manually adding FCombatStat structs to the component's CombatStats array.

These two methods can be combined to an extent by providing a template first (which clears the array of stats and refills it with the template values), and then editing the generated stat structs. This allows reuse of generic stat templates while still allowing customization on an actor-by-actor basis.
> For example, you could create a Stat Template for generic combat actors that has the DamageDone, DamageTaken, HealingDone, and HealingTaken stats all set to 1, and use it as a base for all enemies, and then for an enemy that is intended to take extra healing, you can just modify the HealingTaken stat for that specific actor to be 1.5.

**[⬆ Back to Top](#top)**