#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityFunctionLibrary.generated.h"

UCLASS()
class SAIYORAV4_API UAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static void RegisterNewHitbox(class UHitbox* Hitbox);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", meta = (NativeMakeFunc))
	static FAbilityOrigin MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin);
	
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
    static bool PredictLineTrace(AActor* Shooter, float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel,
    	TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateLineTraceTarget(AActor* Shooter, FAbilityOrigin const& Origin, AActor* Target, float const TraceLength,
		TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const& ActorsToIgnore);

private:

	static TMap<class UHitbox*, FRewindRecord> Snapshots;
	static const float CamTraceLength;
	static const float MaxLagCompensation;
	static const float SnapshotInterval;
	UFUNCTION()
	static void CreateSnapshot();
	static FTimerHandle SnapshotHandle;
	static FTransform RewindHitbox(UHitbox* Hitbox, float const Ping);
};
