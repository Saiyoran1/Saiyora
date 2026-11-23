#pragma once
#include "DamageEnums.generated.h"

UENUM(BlueprintType)
enum class EHealthEventType : uint8
{
    None = 0,
    Damage = 1,
    Healing = 2,
    Absorb = 3,
};

UENUM(BlueprintType)
enum class EElementalSchool : uint8
{
    None = 0,
    Physical = 1,
    Fire = 2,
    Water = 3,
    Sky = 4,
    Earth = 5,
    Military = 6,
};

UENUM(BlueprintType)
enum class EEventHitStyle : uint8
{
    None = 0,
    Direct = 1,
    Area = 2,
    Chronic = 3,
    Authority = 4,
};

UENUM(BlueprintType)
enum class ELifeStatus : uint8
{
    Invalid = 0,
    Alive = 1,
    Dead = 2,
};

UENUM(BlueprintType)
enum class EKillCountType : uint8
{
    None = 0,
    Player = 1,
    Trash = 2,
    Boss = 3,
};

UENUM(BlueprintType)
enum class EHealthEventFailReason : uint8
{
    None UMETA(Hidden),
    //Tried to apply a health event on a client.
    NetRole,
    //Source of the health event wasn't valid.
    InvalidSource,
    //Actor applying the health event wasn't valid.
    InvalidAppliedBy,
    //Event type or amount was invalid.
    InvalidEvent,
    //Target can't receive this type of health event.
    CantTakeEventType,
    //Target was dead.
    Dead,
    //Target was an NPC that was uninitialized or resetting.
    InactiveNPC,
    //Custom restriction to outgoing health events.
    OutgoingRestriction,
    //Custom restriction to incoming health events.
    IncomingRestriction,
};