#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "PredictableProjectile.h"
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

//Trace Prediction
	
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
    static bool PredictLineTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, bool const bHostile,
    	TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateLineTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, AActor* Target, float const TraceLength,
		bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiLineTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, bool const bHostile, TArray<AActor*> const& ActorsToIgnore,
		int32 const TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiLineTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, TArray<AActor*> const& Targets,
		float const TraceLength, bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, float const TraceRadius, bool const bHostile,
		TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, AActor* Target, float const TraceLength,
		float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, float const TraceRadius, bool const bHostile,
		TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, TArray<AActor*> const& Targets, float const TraceLength,
		float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereSightTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, float const TraceRadius, bool const bHostile,
		TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereSightTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, AActor* Target, float const TraceLength,
		float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereSightTrace(class ASaiyoraPlayerCharacter* Shooter, float const TraceLength, float const TraceRadius, bool const bHostile,
		TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereSightTrace(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, TArray<AActor*> const& Targets, float const TraceLength,
		float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore);

//Projectile Prediction

	static void RegisterClientProjectile(APredictableProjectile* Projectile);
	static void ReplaceProjectile(APredictableProjectile* ServerProjectile);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* PredictProjectile(UCombatAbility* Ability, class ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass,
		FAbilityOrigin& OutOrigin);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* ValidateProjectile(UCombatAbility* Ability, class ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass,
		FAbilityOrigin const& Origin);

private:

	static TMap<class UHitbox*, FRewindRecord> Snapshots;
	static const float CamTraceLength;
	static const float MaxLagCompensation;
	static const float SnapshotInterval;
	static const float RewindTraceRadius;
	static const float AimToleranceDegrees;
	UFUNCTION()
	static void CreateSnapshot();
	static FTimerHandle SnapshotHandle;
	static FTransform RewindHitbox(UHitbox* Hitbox, float const Ping);
	static TMultiMap<FPredictedTick, APredictableProjectile*> FakeProjectiles;
};
