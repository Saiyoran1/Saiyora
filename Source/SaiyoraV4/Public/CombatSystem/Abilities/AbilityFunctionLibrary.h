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

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* PredictProjectile(UCombatAbility* Ability, class ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass,
		FAbilityOrigin& OutOrigin);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* ValidateProjectile(UCombatAbility* Ability, class ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass,
		FAbilityOrigin const& Origin);

private:

	static const float CamTraceLength;
	static const float RewindTraceRadius;
	static const float AimToleranceDegrees;

//Helpers

	static void RewindRelevantHitboxes(class ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, TArray<AActor*> const& Targets,
		TArray<AActor*> const& ActorsToIgnore, TMap<class UHitbox*, FTransform>& ReturnTransforms);
	static void UnrewindHitboxes(TMap<class UHitbox*, FTransform> const& ReturnTransforms);
	static float GetCameraTraceMaxRange(FVector const& CameraLoc, FVector const& AimDir, FVector const& OriginLoc, float const TraceRange);
};
