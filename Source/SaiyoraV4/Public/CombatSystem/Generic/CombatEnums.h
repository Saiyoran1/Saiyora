#pragma once

//What version of the world a given actor exists in.
//Actors in different planes from each other may treat combat events differently and see visually distinct versions of the world.
UENUM(BlueprintType)
enum class ESaiyoraPlane : uint8
{
	//In both planes. Considered same-plane when compared to all other planes except Neither.
	Both,
	//In neither plane. Considered x-plane when compared to all other planes.
	Neither,
	Ancient,
	Modern,
};

//Filter enum for determining what actors to consider for a given action.
//For example, when dealing AoE damage, we may want to only damage enemies with a specific plane relationship to the attacking actor.
UENUM(BlueprintType)
enum class ESaiyoraPlaneFilter : uint8
{
	//No filter, include actors in any plane.
	None,
	//Only include actors in the same plane as the querier.
	SamePlane,
	//Only include actors xplane from the querier.
	XPlane,
};

//The direction of a combat event relative to a given actor, such as damage, buff application, or threat.
UENUM(BlueprintType)
enum class ECombatEventDirection : uint8
{
	//The event is being applied TO the actor in question.
	Incoming,
	//The event is being applied BY the actor in question.
	Outgoing,
};

//Determines how a combat modifier should adjust a given value.
UENUM(BlueprintType)
enum class EModifierType : uint8
{
	//Default value, the modifier is not relevant to the given event or value.
	Invalid,
	//The modifier should be added to the value in question.
	Additive,
	//The modifier should multiply the value in question.
	Multiplicative,
};

//Determines what "team" a given actor is on in combat.
UENUM(BlueprintType)
enum class EFaction : uint8
{
	//This actor can be considered friendly or unfriendly to players. FactionFilters for a given event determine how to interact with the Neutral faction.
	Neutral,
	//This actor is considered unfriendly to players. NPCs are enemies by default.
	Enemy,
	//This actor is friendly to players. Players are friendly by default.
	Friendly,
};

//Filter enum for determining what actors to consider for a given action.
//For example, when using a healing ability, an actor may only want to heal other actors that are considered friendly.
UENUM(BlueprintType)
enum class ESaiyoraFactionFilter : uint8
{
	//No filter, include actors of any faction.
	None,
	//Only include actors of the same faction as the querier, including neutral actors.
	SameFaction,
	//Only include actors of the same faction as the querier, excluding neutral actors.
	SameFactionExcludeNeutral,
	//Only include actors of the opposite faction, including neutral actors.
	OppositeFaction,
	//Only include actors of the opposite faction, excluding neutral actors.
	OppositeFactionExcludeNeutral,
};
