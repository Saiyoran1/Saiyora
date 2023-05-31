<a name="top"></a>
**[Back to Root](/README.md)**

# Resources

Resources represent things like mana, energy, or rage in typical RPG systems. In Saiyora, this also notably includes ammo for weapons. The UResourceHandler component maintains replicated UResource objects representing the different resources a character has. In the case of players, this usually only includes ammo and one additional resource from the player's ancient specialization, but the system has the capacity to handle many resources for an individual actor should the need arise.

Resources provide Blueprint callbacks for PreInitializeResource, PreDeactivateResource, and PostResourceChanged, which can be used to create resources with behavior such as regeneration or additional effects on reaching certain thresholds. 

Modification of resources outside of ability costs must occur only on the server, and is a modifiable event similar to damage or healing. Ability costs support prediction, and will occur on the client as well as the server.

## Stat Binding

Resource initialization has the option for stat binding for custom minimum and maximum values. Normal initialization simply sets the resource's minimum and maximum values to either the class default value or a custom value provided in an initialization struct. The alternative is to provide a GameplayTag starting with "Stat" for binding the maximum and minimum values. In these cases, the resource will subscribe to those stat values and update the min and max values dynamically when those stats change. Current value changes proportionally when max or min values change.

## Resource Prediction

When ability usage happens for a non-server player, resource costs are predictively subtracted on the client before being confirmed on the server. This is part of the larger prediction system for abilities, and includes support for misprediction. Essentially, client-side resources display their current value as the sum of their last server-confirmed value and all predictions made by that client since that confirmed value was last replicated. This prevents clients from using abilities very quickly that they won't actually have the resources for and having to be rolled back constantly.

The Resource Handler keeps a record of all resource cost predictions that haven't been confirmed, and after the server confirms an ability use and the associated costs, the resource handler will adjust any predictions that were incorrect. In addition, the server updating the resource values themselves will also clear out predictions when the new corrected value replicates. This duplication of logic means that there will never be a situation where the client has received confirmation of a resource cost (either through replication or the server ability confirmation RPC) and still has an incorrect resource value.

The whole prediction/rollback system for abilities is detailed in the Abilities section, as many values such as ability cooldowns, global cooldowns, and costs follow similar logic.

**[⬆ Back to Top](#top)**