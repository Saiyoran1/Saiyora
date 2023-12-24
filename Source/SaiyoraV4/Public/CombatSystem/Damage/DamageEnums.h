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
