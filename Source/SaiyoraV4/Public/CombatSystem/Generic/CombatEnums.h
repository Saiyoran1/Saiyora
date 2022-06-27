#pragma once

UENUM(BlueprintType)
enum class ESaiyoraPlane : uint8
{
 None = 0,
 Ancient = 1,
 Modern = 2,
 Both = 3,
 Neither = 4,
};

UENUM(BlueprintType)
enum class ECombatEventDirection : uint8
{
 None = 0,
 Incoming = 1,
 Outgoing = 2,
};

UENUM(BlueprintType)
enum class EModifierType : uint8
{
 Invalid = 0,
 Additive = 1,
 Multiplicative = 2,
};

UENUM(BlueprintType)
enum class ECombatParamType : uint8
{
 None,
 Bool,
 Int,
 Float,
 Object,
 Class,
 Vector,
 Rotator,
 String,
};

UENUM(BlueprintType)
enum class EFaction : uint8
{
 None = 0,
 Enemy = 1,
 Neutral = 2,
 Player = 3,
};