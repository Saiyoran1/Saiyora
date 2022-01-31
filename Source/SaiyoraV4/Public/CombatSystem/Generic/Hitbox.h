#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "SaiyoraEnums.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Hitbox.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UBoxHitbox : public UBoxComponent
{
	GENERATED_BODY()
public:
	static const int32 SnapshotBufferSize;
	static const float MaxLagCompTime;
	UBoxHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RewindByTime(float const Ping);
	void Unrewind();
private:
	void UpdateFactionCollision(EFaction const NewFaction);
	TArray<TTuple<float, FTransform>> Snapshots;
	FTransform TransformNoRewind;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API USphereHitbox : public USphereComponent
{
	GENERATED_BODY()
public:
	USphereHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	void UpdateFactionCollision(EFaction const NewFaction);
	TArray<TTuple<float, FTransform>> Snapshots;
};

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UCapsuleHitbox : public UCapsuleComponent
{
	GENERATED_BODY()
public:
	UCapsuleHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	void UpdateFactionCollision(EFaction const NewFaction);
	TArray<TTuple<float, FTransform>> Snapshots;
};
