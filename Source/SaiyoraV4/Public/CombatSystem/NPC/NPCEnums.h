#pragma once
#include "CoreMinimal.h"
#include "NPCEnums.generated.h"

UENUM(BlueprintType)
enum class ENPCCombatBehavior : uint8
{
	None,
	Patrolling,
	Combat,
	Resetting,
};

//This enum is used to determine what an NPC Ability's location rules are relative to.
UENUM(BlueprintType)
enum class ELocationRuleReferenceType : uint8
{
	//No location rules will be applied, the ability can be cast anywhere.
	None,
	//Location rules will be relative to a provided target actor.
	Actor,
	//Location rules will be relative to a provided target location.
	Location,
};