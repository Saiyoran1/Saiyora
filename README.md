# SaiyoraV4

This project is intended to be a game in the spirit of World of Warcraft's Challenge Mode feature, reimagined for a third-person shooter game. As such, it focuses on speedrunning developer-created PvE dungeons with groups of 1-5 players for rankings on a speed leaderboard.
The main design goals include:
- Varied and interesting dungeon encounters that last from 5-20 minutes.
- Deep class customization through a 2-action bar system and per-ability talent selections.
- A focus on speed, facilitated by a lack of "anti-big-pull mechanics" like those commonly seen in WoW's M+ system. Size of pulls should only be limited by execution and ability, not by arbitrary CC requirements or affixes.

COMBAT
The combat system is focused around spellcasting and shooting from a third person perspective. A big inspiration in combat fluidity is Spellbreak.
Combat for players involves two action bars: the Ancient bar and the Modern bar.
The Ancient bar is filled by 7 spells determined by the player's primary specialization, which form a core DPS/healing rotation and provide a few passives and utility functions to supplement this.
The Modern bar is filled by a weapon and 6 spells that the player can choose from a pool of unlocked abilities. Modern spells range from damage to utility to CC to survivability.
Each ability in both bars comes with 2 unlockable variations that can be chosen rather than the base ability, for a ton of customization. Abilities, talents, and weapon can NOT be changed in a dungeon run, only in the waiting period before a run starts or in the lobby menu.
Swapping between bars is locked behind the game concept of Planes, which are different variations of the world that exist simultaneously. Swapping Planes swaps the player to the bar associated with that Plane.
Plane swapping can happen by using certain player abilities, interacting with pressure points in dungeons, or through a number of unique mechanics from mobs or objects in dungeons.

On a technical level, abilities come from two primary classes:
UCombatAbility: Used for NPC abilities, can only be activated from the server, and have their functionality defined by the ServerTick and SimulatedTick Blueprint events.
UPlayerCombatAbility: Used for player abilities, can be predictively activated by the owning client, and have functionality further split into PredictedTick, ServerTick, and SimulatedTick. PlayerCombatAbilities support prediction of charge expenditure, resource cost expenditure, global cooldown, and cast state.
NPCs use a UAbilityHandler component which supports all of the primary functionality needed for using abilities. 
This includes:
- Adding and Removing Abilities (and replicating added and removed abilities)
- Ability Usage (server-authoritative)
- Global Cooldown
- Casting State (replicated)
- Generic Modifiers to Ability Resource Costs, Cast Length, Cooldown Length, Global Cooldown Length
- Class-Specific Modifiers to Resource Costs, Cast Length, Cooldown Length, Global Cooldown Length, Max Charges, Charges Per Cast, Charges Per Cooldown
- Conditional (context-specific) modifiers to Resource Costs, Cast Length, Cooldown Length, Global Cooldown Length, Charges Per Cast, Charges Per Cooldown
- Adding and Removing Custom Restrictions for Ability Usage
- Cancelling Abilities (replicated)
- Interrupting Abilities (replicated)

The PlayerAbilityHandler adds:
- Unlocking Abilities and Talents
- Talent Selection
- Specialization (and associated loadouts)
- Limited Ability Loadouts (in line with the 2-bar system)
- Prediction of Ability Usage, Global Cooldown, Casting State, Charge Expenditure, Resource Cost Expenditure, Cancelling
- Misprediction Callback to handle mispredicted effects


Developed with Unreal Engine 4
