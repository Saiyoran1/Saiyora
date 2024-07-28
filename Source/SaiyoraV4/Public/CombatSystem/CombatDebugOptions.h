#pragma once
#include "CoreMinimal.h"
#include "CombatDebugOptions.generated.h"

class APredictableProjectile;
class UNPCAbility;
class UHitbox;
struct FNPCAbilityTokens;
struct FAbilityEvent;
struct FRotationDebugInfo;

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
};
