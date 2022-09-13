# Game Overview

Saiyora is a third person shooter game with RPG elements that focuses on speedrunning developer-created "dungeons" with a group of 1-4 players. Each dungeon exists in two different Planes: the Ancient and the Modern, which players can shift between to change their abilities, handle enemy mechanics, and traverse the dungeon. The Ancient Plane grants players a set of abilities that allow them to physically manifest magic to attack and heal with, while the Modern Plane grants a gun as a primary source of damage and abilities that enhance the player's defense, utility, or mobility.

The game is entirely PvE (player-vs-environment), and is inspired by the Challenge Mode system of World of Warcraft, which existed briefly during the "Mists of Pandaria" and "Warlords of Draenor" expansions. The ever-evolving strategy and deep optimization that the Challenge Mode system promoted are the main pillars I hope to carry over into my own game.

Saiyora is created in Unreal Engine 5, with most core systems and base classes written in C++, and most content created as Blueprint classes derived from those classes. All art assets are placeholder, sourced either from the Unreal Marketplace, free sites like Mixamo, or downloaded with the engine itself.  

## Technical Highlights

Some features that Saiyora has that I'm particularly proud of:

- A custom ability system featuring client prediction, reconciliation, and validation of ability usage, global cooldowns, ability cooldowns, casting state, and resource expenditure.  
- Replication and prediction of Root Motion Handlers and other custom moves, integrated with the ability system and Unreal's Character Movement Component.  
- Flexible systems for modifying and restricting combat events such as damage, healing, death, buff application, crowd control, ability usage, and threat generation using function delegates as a representation of contextual conditions.  
- Server-side rewind for both hitscan and projectile abilities, as well as projectile synchronization between predicting clients and the server.  
- A custom buff system that allows reusable functionality between buffs and complete control over duration, stacking, and overlapping behavior during gameplay.  

Below are links to the README files for each of the main parts of Saiyora's combat system, featuring more in depth explanations on all the different functionality I have implemented.

> [General Concepts](Source/SaiyoraV4/Public/CombatSystem/Generic/GeneralConceptsREADME.md)  
> [Planes]()  
> [Health]()  
> [Buffs]()  
> [Crowd Control]()  
> [Stats]()  
> [Resources]()  
> [Abilities]()  
> [Movement]()  
