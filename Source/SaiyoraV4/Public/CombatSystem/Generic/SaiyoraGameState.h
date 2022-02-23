#pragma once
#include "AbilityStructs.h"
#include "SaiyoraPlayerController.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

USTRUCT()
struct FRewindRecord
{
	GENERATED_BODY()

	TArray<TTuple<float, FTransform>> Transforms;
};

UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

private:
	float WorldTime = 0.0f;
	void UpdateClientWorldTime(float const ServerTime);
public:
	ASaiyoraGameState();
	virtual float GetServerWorldTimeSeconds() const override;
	friend void ASaiyoraPlayerController::ClientHandleWorldTimeReturn_Implementation(float const ClientTime, float const ServerTime);
protected:
	virtual void Tick(float DeltaSeconds) override;

public:

	void RegisterNewHitbox(class UHitbox* Hitbox);
	void RegisterClientProjectile(APredictableProjectile* Projectile);
	void ReplaceProjectile(APredictableProjectile* ServerProjectile);
	FTransform RewindHitbox(UHitbox* Hitbox, float const Ping);

private:

	static const float SnapshotInterval;
	static const float MaxLagCompensation;
	TMap<class UHitbox*, FRewindRecord> Snapshots;
	UFUNCTION()
	void CreateSnapshot();
	FTimerHandle SnapshotHandle;
	TMultiMap<FPredictedTick, APredictableProjectile*> FakeProjectiles;
};
