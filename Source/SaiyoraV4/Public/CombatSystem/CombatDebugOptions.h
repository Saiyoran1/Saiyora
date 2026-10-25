#pragma once
#include "CoreMinimal.h"
#include "CombatDebugOptions.generated.h"

struct FBuffRemoveEvent;
class UBuffHandler;
class FBuffRestriction;
struct FBuffApplyEvent;
class APredictableProjectile;
class UNPCAbility;
class UHitbox;
struct FNPCAbilityTokens;
struct FAbilityEvent;
struct FRotationDebugInfo;

#pragma region Macros

//Debug logging for buff application
#if !UE_BUILD_SHIPPING
	#define DEBUG_LOG_BUFF_APPLY(Handler, Event) \
	if (CombatDebug_DA->bEnableBuffDebugLogging) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyEvent(Handler, Event); \
	}
#else
	#define DEBUG_LOG_BUFF_APPLY(Handler, Event)
#endif

//Debug logging for buff application failure
#if !UE_BUILD_SHIPPING
	#define DEBUG_LOG_BUFF_APPLY_FAIL(Handler, Event) \
	if (CombatDebug_DA->bEnableBuffDebugLogging) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffApplyFailure(Handler, Event); \
	}
#else
	#define DEBUG_LOG_BUFF_APPLY_FAIL(Handler, Event)
#endif

//Debug logging for buff removal
#if !UE_BUILD_SHIPPING
	#define DEBUG_LOG_BUFF_REMOVE(Handler, Event) \
	if (CombatDebug_DA->bEnableBuffDebugLogging) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveEvent(Handler, Event); \
	}
#else
	#define DEBUG_LOG_BUFF_REMOVE(Handler, Event)
#endif

//Debug logging for buff removal failure
#if !UE_BUILD_SHIPPING
	#define DEBUG_LOG_BUFF_REMOVE_FAIL(Handler, Event) \
	if (CombatDebug_DA->bEnableBuffDebugLogging) \
	{ \
		CombatDebug_DA->DEBUG_LogBuffRemoveFailure(Handler, Event); \
	}
#else
	#define DEBUG_LOG_BUFF_REMOVE_FAIL(Handler, Event)
#endif

#pragma endregion 

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
	bool bEnableBuffDebugLogging = false;
	
	void DEBUG_LogBuffApplyEvent(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) {}
	void DEBUG_LogBuffApplyFailure(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) {}
	void DEBUG_LogBuffRestriction(const bool bAdded, const FBuffRestriction& Restriction) {}
	void DEBUG_LogBuffRemoveEvent(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) {}
	void DEBUG_LogBuffRemoveFailure(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) {}

#pragma endregion
};
