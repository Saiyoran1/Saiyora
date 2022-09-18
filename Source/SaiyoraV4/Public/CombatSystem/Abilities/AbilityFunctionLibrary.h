#pragma once
#include "CoreMinimal.h"
#include "SaiyoraPlayerCharacter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityFunctionLibrary.generated.h"

class UHitbox;
class ASaiyoraPlayerCharacter;
class APredictableProjectile;
struct FAbilityOrigin;
struct FAbilityTargetSet;
class UCombatAbility;

UCLASS()
class SAIYORAV4_API UAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Abilities", meta = (NativeMakeFunc))
	static FAbilityOrigin MakeAbilityOrigin(const FVector& AimLocation, const FVector& AimDirection, const FVector& Origin);

//Trace Prediction
	
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
    static bool PredictLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
    	const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const bool bHostile, const TArray<AActor*>& ActorsToIgnore,
		const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets,
		const float TraceLength, const bool bHostile, const TArray<AActor*>& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const bool bHostile,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const float TraceRadius, const bool bHostile, const TArray<AActor*>& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const bool bHostile,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength,
		const float TraceRadius, const bool bHostile, const TArray<AActor*>& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const bool bHostile,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const float TraceRadius, const bool bHostile, const TArray<AActor*>& ActorsToIgnore);

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const bool bHostile,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength,
		const float TraceRadius, const bool bHostile, const TArray<AActor*>& ActorsToIgnore);

//Projectile Prediction

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* PredictProjectile(UCombatAbility* Ability, ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass,
		FAbilityOrigin& OutOrigin);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* ValidateProjectile(UCombatAbility* Ability, ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass,
		const FAbilityOrigin& Origin);

private:

	static const float CAMTRACELENGTH;
	static const float REWINDTRACERADIUS;
	static const float AIMTOLERANCEDEGREES;

//Helpers

	static void RewindRelevantHitboxes(const ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets,
		const TArray<AActor*>& ActorsToIgnore, TMap<UHitbox*, FTransform>& ReturnTransforms);
	static void UnrewindHitboxes(const TMap<UHitbox*, FTransform>& ReturnTransforms);
	static float GetCameraTraceMaxRange(const FVector& CameraLoc, const FVector& AimDir, const FVector& OriginLoc, const float TraceRange);
	static FName GetRelevantTraceProfile(const ASaiyoraPlayerCharacter* Shooter, const bool bOverlap, const ESaiyoraPlane TracePlane, const EFaction TraceHostility);
};
