#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "CombatEnums.h"
#include "DamageHandler.h"
#include "DamageStructs.h"
#include "CombatDebugOptions.generated.h"

class UDamageHandler;
class UBuffHandler;
class APredictableProjectile;
class UNPCAbility;
class UHitbox;
struct FNPCAbilityTokens;
struct FAbilityEvent;
struct FRotationDebugInfo;

#pragma region Macros
#if !UE_BUILD_SHIPPING /* Define macros for each debug logging function to easily compile them all out of shipping and access them more easily. */

#pragma region Buffs

	//Debug logging for buff application.
	#define DEBUG_LOG_BUFF_APPLY(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyEvent(Handler, Event); \
	}

	//Debug logging for buff application failure.
	#define DEBUG_LOG_BUFF_APPLY_FAIL(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyFailure(Handler, Event); \
	}

	//Debug logging for buff removal.
	#define DEBUG_LOG_BUFF_REMOVE(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveEvent(Handler, Event); \
	}

	//Debug logging for buff removal failure.
	#define DEBUG_LOG_BUFF_REMOVE_FAIL(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveFailure(Handler, Event); \
	}

	//Debug logging for buff restrictions being added.
	#define DEBUG_LOG_BUFF_RESTRICTION_ADDED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionAdded(Handler, Direction, Restriction); \
	}

	//Debug logging for when adding a buff restriction fails.
	#define DEBUG_LOG_BUFF_RESTRICTION_ADD_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionAddFailed(Handler, Direction, Restriction, FailReason); \
	}

	//Debug logging for buff restriction removal.
	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionRemoved(Handler, Direction, Restriction); \
	}

	//Debug logging for when removing a buff restriction fails.
	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVE_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionRemoveFailed(Handler, Direction, Restriction, FailReason); \
	}

#pragma endregion
#pragma region Health Events

	//Debug logging for health events.
	#define DEBUG_LOG_HEALTH_EVENT(Handler, Event) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEvent(Handler, Event); \
	}

	//Debug logging for health event failure.
	#define DEBUG_LOG_HEALTH_EVENT_FAIL(Handler, Event) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventFailure(Handler, Event); \
	}

	//Debug logging for deaths.
	#define DEBUG_LOG_DEATH(Handler, EventID, bWasPendingKillingBlow) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeath(Handler, EventID, bWasPendingKillingBlow); \
	}

	//Debug logging for when a health event that brings an actor to 0 hp fails to kill them due to a death restriction.
	#define DEBUG_LOG_DEATH_RESTRICTED(Handler, EventID, bWasPendingKillingBlow, bAddedAsPendingKillingBlow, Restriction) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeathRestricted(Handler, EventID, bWasPendingKillingBlow, bAddedAsPendingKillingBlow, Restriction); \
	}

	//Debug logging for respawning.
	#define DEBUG_LOG_RESPAWN(Handler, Resurrection) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogRespawn(Handler, Resurrection); \
	}

	//Debug logging for when a respawn fails.
	#define DEBUG_LOG_RESPAWN_FAILED(Handler, Resurrection, FailReason) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogRespawnFailed(Handler, Resurrection, FailReason); \
	}

	//Debug logging for when a resurrection is offered.
	#define DEBUG_LOG_RESURRECTION_OFFERED(Handler, Resurrection) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogResurrectionOffered(Handler, Resurrection); \
	}

	#define DEBUG_LOG_RESURRECTION_DECISION(Handler, Resurrection, bAccepted) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogResurrectionDecision(Handler, Resurrection, bAccepted); \
	}

	//Debug logging for when a resurrection offer is removed.
	#define DEBUG_LOG_RESURRECTION_REMOVED(Handler, Resurrection) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogResurrectionRemoved(Handler, Resurrection); \
	}

	//Debug logging for health event restrictions being added.
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADDED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventRestrictionAdded(Handler, Direction, Restriction); \
	}

	//Debug logging for when adding a health event restriction fails.
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADD_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventRestrictionAddFailed(Handler, Direction, Restriction, FailReason); \
	}

	//Debug logging for when a health event restriction is removed.
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventRestrictionRemoved(Handler, Direction, Restriction); \
	}

	//Debug logging for when removing a health event restriction fails.
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVE_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventRestrictionRemoveFailed(Handler, Direction, Restriction, FailReason); \
	}

	//Debug logging for health event modifiers being added.
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADDED(Handler, Direction, Modifier) \
	if (CombatDebug_DA->HealthEventModifierLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventModifierAdded(Handler, Direction, Modifier); \
	}

	//Debug logging for when adding a health event modifier fails.
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADD_FAILED(Handler, Direction, Modifier, FailReason) \
	if (CombatDebug_DA->HealthEventModifierLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventModifierAddFailed(Handler, Direction, Modifier, FailReason); \
	}

	//Debug logging for when a health event modifier is removed.
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVED(Handler, Direction, Modifier) \
	if (CombatDebug_DA->HealthEventModifierLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventModifierRemoved(Handler, Direction, Modifier); \
	}

	//Debug logging for when removing a health event modifier fails.
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVE_FAILED(Handler, Direction, Modifier, FailReason) \
	if (CombatDebug_DA->HealthEventModifierLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventModifierRemoveFailed(Handler, Direction, Modifier, FailReason); \
	}

	//Debug logging for death restrictions being added.
	#define DEBUG_LOG_DEATH_RESTRICTION_ADDED(Handler, Restriction) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeathRestrictionAdded(Handler, Restriction); \
	}

	//Debug logging for when adding a death restriction fails.
	#define DEBUG_LOG_DEATH_RESTRICTION_ADD_FAILED(Handler, Restriction, FailReason) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeathRestrictionAddFailed(Handler, Restriction, FailReason); \
	}

	//Debug logging for when a death restriction is removed.
	#define DEBUG_LOG_DEATH_RESTRICTION_REMOVED(Handler, Restriction) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeathRestrictionRemoved(Handler, Restriction); \
	}

	//Debug logging for when removing a death restriction fails.
	#define DEBUG_LOG_DEATH_RESTRICTION_REMOVE_FAILED(Handler, Restriction, FailReason) \
	if (CombatDebug_DA->HealthEventRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeathRestrictionRemoveFailed(Handler, Restriction, FailReason); \
	}

#pragma endregion

#else /* Define all debug logging macros to nothing to compile them out of shipping builds. */

	#define DEBUG_LOG_BUFF_APPLY(Handler, Event)
	#define DEBUG_LOG_BUFF_APPLY_FAIL(Handler, Event)
	#define DEBUG_LOG_BUFF_REMOVE(Handler, Event)
	#define DEBUG_LOG_BUFF_REMOVE_FAIL(Handler, Event)
	#define DEBUG_LOG_BUFF_RESTRICTION_ADDED(Handler, Direction, Restriction)
	#define DEBUG_LOG_BUFF_RESTRICTION_ADD_FAILED(Handler, Direction, Restriction, FailReason)
	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVED(Handler, Direction, Restriction)
	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVE_FAILED(Handler, Direction, Restriction, FailReason)

	#define DEBUG_LOG_HEALTH_EVENT(Handler, Event)
	#define DEBUG_LOG_HEALTH_EVENT_FAIL(Handler, Event)
	#define DEBUG_LOG_DEATH(Handler, EventID, bWasPendingKillingBlow)
	#define DEBUG_LOG_DEATH_RESTRICTED(Handler, EventID, bWasPendingKillingBlow, bAddedAsPendingKillingBlow, Restriction)
	#define DEBUG_LOG_RESPAWN(Handler, Resurrection)
	#define DEBUG_LOG_RESPAWN_FAILED(Handler, Resurrection, FailReason)
	#define DEBUG_LOG_RESURRECTION_OFFERED(Handler, Resurrection)
	#define DEBUG_LOG_RESURRECTION_DECISION(Handler, Resurrection, bAccepted)
	#define DEBUG_LOG_RESURRECTION_REMOVED(Handler, Resurrection)
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADDED(Handler, Direction, Restriction)
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_ADD_FAILED(Handler, Direction, Restriction, FailReason)
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVED(Handler, Direction, Restriction)
	#define DEBUG_LOG_HEALTH_EVENT_RESTRICTION_REMOVE_FAILED(Handler, Direction, Restriction, FailReason)
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADDED(Handler, Direction, Modifier)
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_ADD_FAILED(Handler, Direction, Modifier, FailReason)
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVED(Handler, Direction, Modifier)
	#define DEBUG_LOG_HEALTH_EVENT_MODIFIER_REMOVE_FAILED(Handler, Direction, Modifier, FailReason)
	#define DEBUG_LOG_DEATH_RESTRICTION_ADDED(Handler, Restriction)
	#define DEBUG_LOG_DEATH_RESTRICTION_ADD_FAILED(Handler, Restriction, FailReason)
	#define DEBUG_LOG_DEATH_RESTRICTION_REMOVED(Handler, Restriction)
	#define DEBUG_LOG_DEATH_RESTRICTION_REMOVE_FAILED(Handler, Restriction, FailReason)

#endif
#pragma endregion 

UENUM()
enum class EDebugLogVerbosity
{
	Off,
	Minimal,
	FullDump
};

UCLASS()
class SAIYORAV4_API UCombatDebugOptions : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	bool bLogAbilityEvents = false;
	void LogAbilityEvent(const AActor* Actor, const FAbilityEvent& Event);

	UPROPERTY(EditAnywhere, Category = "Abilities")
	bool bDisplayTokenInformation = false;
	void DisplayTokenInfo(const TMap<TSubclassOf<UNPCAbility>, FNPCAbilityTokens>& Tokens);

	UPROPERTY(EditAnywhere, Category = "AI")
	bool bDisplayClaimedLocations = false;
	void DisplayClaimedLocations(const TMap<AActor*, FVector>& Locations);

	UPROPERTY(EditAnywhere, Category = "AI")
	bool bDrawRotationBehavior = false;
	void DrawRotationBehavior(const AActor* Actor, const FRotationDebugInfo& DebugInfo);

	UPROPERTY(EditAnywhere, Category = "Net")
	bool bDrawRewindHitboxes = false;
	void DrawRewindHitbox(const UHitbox* Hitbox, const FTransform& PreviousTransform);

	UPROPERTY(EditAnywhere, Category = "Net")
	bool bDrawHiddenProjectiles = false;
	void DrawHiddenProjectile(const APredictableProjectile* Projectile);

#pragma region Buffs

public:

	UPROPERTY(EditAnywhere, Category = "Buffs")
	EDebugLogVerbosity BuffLoggingVerbosity = EDebugLogVerbosity::Off;
	UPROPERTY(EditAnywhere, Category = "Buffs")
	EDebugLogVerbosity BuffRestrictionLoggingVerbosity = EDebugLogVerbosity::Off;

#if !UE_BUILD_SHIPPING
	
	void DEBUG_LogBuffApplyEvent(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) const;
	void DEBUG_LogBuffApplyFailure(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) const;
	void DEBUG_LogBuffRestrictionAdded(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction) const;
	void DEBUG_LogBuffRestrictionAddFailed(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction, const FString& FailReason) const;
	void DEBUG_LogBuffRestrictionRemoved(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction) const;
	void DEBUG_LogBuffRestrictionRemoveFailed(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction, const FString& FailReason) const;
	void DEBUG_LogBuffRemoveEvent(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) const;
	void DEBUG_LogBuffRemoveFailure(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) const;

#endif
#pragma endregion
#pragma region Health Events

public:

	UPROPERTY(EditAnywhere, Category = "Health Events")
	EDebugLogVerbosity HealthEventLoggingVerbosity = EDebugLogVerbosity::Off;
	UPROPERTY(EditAnywhere, Category = "Health Events")
	EDebugLogVerbosity HealthEventRestrictionLoggingVerbosity = EDebugLogVerbosity::Off;
	UPROPERTY(EditAnywhere, Category = "Health Events")
	EDebugLogVerbosity HealthEventModifierLoggingVerbosity = EDebugLogVerbosity::Off;

#if !UE_BUILD_SHIPPING

	void DEBUG_LogHealthEvent(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const;
	void DEBUG_LogHealthEventFailure(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const;
	void DEBUG_LogDeath(const UDamageHandler* Handler, const int EventID, const bool bWasPendingKillingBlow) const;
	void DEBUG_LogDeathRestricted(const UDamageHandler* Handler, const int EventID, const bool bWasPendingKillingBlow, const bool bAddedAsPendingKillingBlow, const FDeathRestriction& Restriction) const;
	void DEBUG_LogRespawn(const UDamageHandler* Handler, const FPendingResurrection& Resurrection) const;
	void DEBUG_LogRespawnFailed(const UDamageHandler* Handler, const FPendingResurrection& Resurrection, const FString& FailReason) const;
	void DEBUG_LogResurrectionOffered(const UDamageHandler* Handler, const FPendingResurrection& Resurrection) const;
	void DEBUG_LogResurrectionDecision(const UDamageHandler* Handler, const FPendingResurrection& Resurrection, const bool bAccepted) const;
	void DEBUG_LogResurrectionRemoved(const UDamageHandler* Handler, const FPendingResurrection& Resurrection) const;
	void DEBUG_LogHealthEventRestrictionAdded(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction) const;
	void DEBUG_LogHealthEventRestrictionAddFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction, const FString& FailReason) const;
	void DEBUG_LogHealthEventRestrictionRemoved(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction) const;
	void DEBUG_LogHealthEventRestrictionRemoveFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction, const FString& FailReason) const;
	void DEBUG_LogHealthEventModifierAdded(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier) const;
	void DEBUG_LogHealthEventModifierAddFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier, const FString& FailReason) const;
	void DEBUG_LogHealthEventModifierRemoved(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier) const;
	void DEBUG_LogHealthEventModifierRemoveFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier, const FString& FailReason) const;
	void DEBUG_LogDeathRestrictionAdded(const UDamageHandler* AppliedToHandler, const FDeathRestriction& Restriction) const;
	void DEBUG_LogDeathRestrictionAddFailed(const UDamageHandler* AppliedToHandler, const FDeathRestriction& Restriction, const FString& FailReason) const;
	void DEBUG_LogDeathRestrictionRemoved(const UDamageHandler* AppliedToHandler, const FDeathRestriction& Restriction) const;
	void DEBUG_LogDeathRestrictionRemoveFailed(const UDamageHandler* AppliedToHandler, const FDeathRestriction& Restriction, const FString& FailReason) const;
	
#endif
#pragma endregion 
};
