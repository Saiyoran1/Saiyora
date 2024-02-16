#pragma once

UENUM(BlueprintType)
enum class EBuffType : uint8
{
	//Typically a "positive" status effect
	Buff,
	//Typically a "negative" status effect
	Debuff,
	//Status effect not displayed on player UI
	HiddenBuff,
};

UENUM(BlueprintType)
enum class EBuffApplicationOverrideType : uint8
{
	//No override, stacking and refreshing behavior are dictated by buff class params
	None,
	//Override will replace the existing stacks or duration
	Normal,
	//Override will add to the existing stacks or duration
	Additive,
};

UENUM(BlueprintType)
enum class EBuffExpireReason : uint8
{
	//Default value, not sure if this needs to exist (do OnReps get called for values that haven't changed since object creation?)
	Invalid,
	//Buff expired naturally due to duration
	TimedOut,
	//Buff was manually removed by a game mechanic
	Dispel,
	//Target died and buff can not apply to dead targets
	Death,
	//Target left combat and buff does not persist outside of combat
	Combat,
};

UENUM(BlueprintType)
enum class EBuffApplyAction : uint8
{
	//Failed to apply the buff
	Failed,
	//Created a brand new buff instance
	NewBuff,
	//Stacked an existing buff instance
	Stacked,
	//Refreshed an existing buff instance
	Refreshed,
	//Stacked and refreshed an existing buff instance
	StackedAndRefreshed,
};

//Unsure if this enum needs to exist at all
UENUM(BlueprintType)
enum class EBuffStatus : uint8
{
	Spawning,
	Active,
	Removed,
};

UENUM(BlueprintType)
enum class EBuffApplyFailReason : uint8
{
	//Tried to apply a buff on a client
	NetRole,
	//Buff class supplied wasn't valid
	InvalidClass,
	//Source of the buff wasn't valid
	InvalidSource,
	//Actor applying the buff wasn't valid
	InvalidAppliedBy,
	//Actor the buff was applied to can't receive buffs
	InvalidAppliedTo,
	//Custom restriction to outgoing buffs
	OutgoingRestriction,
	//Custom restriction to incoming buffs
	IncomingRestriction,
	//Target was dead
	Dead,
	//Target can not receive damage
	ImmuneToDamage,
	//Target can not receive healing
	ImmuneToHealing,
	//Target did not have stat or that stat wasn't modifiable for this target
	StatNotModifiable,
	//Target is immune to this kind of CC
	ImmuneToCc,
	//Target does not have a threat table
	NoThreatTable,
	//Target does not have a movement component
	NoMovement,
	//Target can not be in a threat table
	InvalidThreatTarget,
	//Existing buff couldn't stack or refresh, and the buff wasn't duplicable
	NoStackRefreshOrDuplicate,
};