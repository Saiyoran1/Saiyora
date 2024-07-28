#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "WorldSubsystem.h"
#include "CombatNetSubsystem.generated.h"

class ASaiyoraPlayerCharacter;
class APredictableProjectile;
class UCombatDebugOptions;
class UHitbox;

#pragma region Structs

USTRUCT()
struct FRewindRecord
{
	GENERATED_BODY()

	TArray<TTuple<float, FTransform>> Transforms;
};

USTRUCT()
struct FPredictedTickProjectiles
{
	GENERATED_BODY()

	int32 IDCounter = 0;
	UPROPERTY()
	TMap<int32, APredictableProjectile*> Projectiles;
};

USTRUCT()
struct FPlayerProjectileInfo
{
	GENERATED_BODY()

	TMap<FPredictedTick, FPredictedTickProjectiles> TickProjectiles;
};

#pragma endregion 

//Subsystem for handling combat networking, including projectile prediction, hitbox rewinding, and 
UCLASS()
class SAIYORAV4_API UCombatNetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

#pragma region Core

public:

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:

	UPROPERTY()
	UCombatDebugOptions* DebugOptions;

#pragma endregion 
#pragma region Hitbox Rewinding

public:

	void RegisterNewHitbox(UHitbox* Hitbox);
	FTransform RewindHitbox(UHitbox* Hitbox, const float Timestamp);

private:

	static constexpr float SnapshotInterval = 0.03f;
	static constexpr float MaxLagCompensation = 0.2f;
	TMap<UHitbox*, FRewindRecord> Snapshots;
	UFUNCTION()
	void CreateSnapshot();
	FTimerHandle SnapshotHandle;

#pragma endregion
#pragma region Projectiles

public:

	//Get a projectile ID that is specific to an FPredictedTick for a given player.
	//This is used by both the client and server, and depends on the same number of projectiles being initialized in the same order for a given FPredictedTick.
	int32 GetNewProjectileID(ASaiyoraPlayerCharacter* Player, const FPredictedTick& Tick);
	//Called on the predicting client to save a mapping to a predicted projectile. The server's replicated projectile will use this to replace the client projectile when it has finished catching up.
	void RegisterClientProjectile(ASaiyoraPlayerCharacter* Player, APredictableProjectile* Projectile);
	//Called by the server's projectile after replicating to the client to destroy the predicted projectile and replace it with the server projectile.
	void ReplaceProjectile(APredictableProjectile* AuthProjectile);

private:

	UPROPERTY()
	TMap<ASaiyoraPlayerCharacter*, FPlayerProjectileInfo> PlayerProjectiles;

#pragma endregion
};
