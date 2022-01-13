#pragma once

UENUM(BlueprintType)
enum class EBuffType : uint8
{
 Buff = 0,
 Debuff = 1,
 HiddenBuff = 2,
};

UENUM(BlueprintType)
enum class EBuffApplicationOverrideType : uint8
{
 None = 0,
 Normal = 1,
 Additive = 2,
};

UENUM(BlueprintType)
enum class EBuffExpireReason : uint8
{
 Invalid = 0,
 TimedOut = 1,
 Dispel = 2,
 Death = 3,
 Absolute = 4,
};

UENUM(BlueprintType)
enum class EBuffApplyAction : uint8
{
 Failed = 0,
 NewBuff = 1,
 Stacked = 2,
 Refreshed = 3,
 StackedAndRefreshed = 4,
};

UENUM(BlueprintType)
enum class EBuffStatus : uint8
{
 Spawning = 0,
 Active = 1,
 Removed = 2,
};

UENUM(BlueprintType)
enum class EBuffParamType : uint8
{
 Invalid = 0,
 FearVectorX = 1,
 FearVectorY = 2,
 FearVectorZ = 3,
 SpellSchoolEnum = 4,
 SpellClass = 5,
};

UENUM(BlueprintType)
enum class EBuffRestrictionType : uint8
{
 None,
 Outgoing,
 Incoming
};