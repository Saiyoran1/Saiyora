#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "CombatEnums.h"
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

	//Debug logging for buff application
	#define DEBUG_LOG_BUFF_APPLY(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyEvent(Handler, Event); \
	}

	//Debug logging for buff application failure
	#define DEBUG_LOG_BUFF_APPLY_FAIL(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyFailure(Handler, Event); \
	}

	//Debug logging for buff removal
	#define DEBUG_LOG_BUFF_REMOVE(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveEvent(Handler, Event); \
	}

	//Debug logging for buff removal failure
	#define DEBUG_LOG_BUFF_REMOVE_FAIL(Handler, Event) \
	if (CombatDebug_DA->BuffLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveFailure(Handler, Event); \
	}

	//Debug logging for buff restrictions being added
	#define DEBUG_LOG_BUFF_RESTRICTION_ADDED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionAdded(Handler, Direction, Restriction); \
	}

	#define DEBUG_LOG_BUFF_RESTRICTION_ADD_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionAddFailed(Handler, Direction, Restriction, FailReason); \
	}

	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVED(Handler, Direction, Restriction) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionRemoved(Handler, Direction, Restriction); \
	}

	#define DEBUG_LOG_BUFF_RESTRICTION_REMOVE_FAILED(Handler, Direction, Restriction, FailReason) \
	if (CombatDebug_DA->BuffRestrictionLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRestrictionRemoveFailed(Handler, Direction, Restriction, FailReason); \
	}

#pragma endregion
#pragma region Health Events

	//Debug logging for health events
	#define DEBUG_LOG_HEALTH_EVENT(Handler, Event) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEvent(Handler, Event); \
	}

	//Debug logging for health event failure
	#define DEBUG_LOG_HEALTH_EVENT_FAIL(Handler, Event) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogHealthEventFailure(Handler, Event); \
	}

	#define DEBUG_LOG_DEATH(Handler, EventID, bWasPendingKillingBlow) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogDeath(Handler, EventID, bWasPendingKillingBlow); \
	}

	#define DEBUG_LOG_RESPAWN(Handler, Resurrection) \
	if (CombatDebug_DA->HealthEventLoggingVerbosity != EDebugLogVerbosity::Off) \
	{ \
		CombatDebug_DA->DEBUG_LogRespawn(Handler, Resurrection); \
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
	#define DEBUG_LOG_RESPAWN(Handler, Resurrection)

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

#if !UE_BUILD_SHIPPING

	void DEBUG_LogHealthEvent(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const;
	void DEBUG_LogHealthEventFailure(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const;
	void DEBUG_LogDeath(const UDamageHandler* Handler, const int EventID, const bool bWasPendingKillingBlow) const;
	void DEBUG_LogRespawn(const UDamageHandler* Handler, const FPendingResurrection& Resurrection) const;
	
#endif
#pragma endregion 
};
