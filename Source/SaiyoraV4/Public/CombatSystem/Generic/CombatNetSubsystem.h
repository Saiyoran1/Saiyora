#pragma once
#include "CoreMinimal.h"
#include "WorldSubsystem.h"
#include "CombatNetSubsystem.generated.h"

class UCombatDebugOptions;
class UHitbox;

USTRUCT()
struct FRewindRecord
{
	GENERATED_BODY()

	TArray<TTuple<float, FTransform>> Transforms;
};

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
};
