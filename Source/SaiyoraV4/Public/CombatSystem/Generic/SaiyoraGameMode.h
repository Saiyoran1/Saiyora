#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "GameFramework/GameMode.h"
#include "SaiyoraGameMode.generated.h"

UCLASS()
class SAIYORAV4_API ASaiyoraGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	void RegisterNewHitbox(class UHitbox* Hitbox);

	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	bool ValidateLineTraceTarget(AActor* Shooter, FAbilityOrigin const& Origin, AActor* Target, float const TraceLength,
		TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const& ActorsToIgnore);
	/*UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	bool ValidateMultiLineTraceTarget(AActor* Shooter, FAbilityOrigin const& Target, float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const ActorsToIgnore);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	void ValidateMultiLineTraceTargets(AActor* Shooter, UPARAM(ref) TArray<FAbilityOrigin>& Targets, float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel,
		TArray<AActor*> const ActorsToIgnore);*/

	static const float CamTraceLength;

private:

	static const float MaxLagCompensation;
	static const float SnapshotInterval;
	
	UPROPERTY()
	TMap<UHitbox*, FRewindRecord> Snapshots;
	UFUNCTION()
	void CreateSnapshot();
	FTimerHandle SnapshotHandle;
	FTransform RewindHitbox(UHitbox* Hitbox, float const Ping);
};
