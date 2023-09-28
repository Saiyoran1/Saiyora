﻿#pragma once
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
class AGroundAttack;

UCLASS()
class SAIYORAV4_API UAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Abilities", meta = (NativeMakeFunc, AutoCreateRefTerm = "AimLocation, AimDirection, Origin"))
	static FAbilityOrigin MakeAbilityOrigin(const FVector& AimLocation, const FVector& AimDirection, const FVector& Origin);

//Trace Prediction

	//Line trace for a single target in front of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
    static bool PredictLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
    	const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	//Line trace for multiple targets in a line that falls within a reasonable cone in front of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore,
		const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets,
		const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	//Sphere trace for a single target in front of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	//Sphere trace for multiple targets in a line that falls within a reasonable cone in front of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength,
		const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	//Trace performed by using a line trace to find a viable target location within a reasonable cone in front of the shot origin, then using a sphere trace to find the first target within a radius of that line that is also within line of sight of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool ValidateSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target, const float TraceLength,
		const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

	//Trace performed by using a line trace to find a viable target location within a reasonable cone in front of the shot origin, then using a sphere trace to find all targets within a radius of that line that are also within line of sight of the shot origin.
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static bool PredictMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
		const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	static TArray<AActor*> ValidateMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength,
		const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore);

//Projectile Prediction

	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* PredictProjectile(UCombatAbility* Ability, ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass,
		const ESaiyoraPlane ProjectilePlane, const EFaction ProjectileHostility, FAbilityOrigin& OutOrigin);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities", meta = (DefaultToSelf = "Ability", HidePin = "Ability"))
	static APredictableProjectile* ValidateProjectile(UCombatAbility* Ability, ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass,
		const ESaiyoraPlane ProjectilePlane, const EFaction ProjectileHostility, const FAbilityOrigin& Origin);

private:

	static constexpr float CamTraceLength = 10000.0f;
	static constexpr float RewindTraceRadius = 300.0f;
	static constexpr float AimToleranceDegrees = 15.0f;

	//Add Spawning

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	static AActor* SpawnAdd(AActor* Summoner, const TSubclassOf<AActor> AddClass, const FTransform& SpawnTransform);

	//Ground Attack

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	static AGroundAttack* SpawnGroundEffect(AActor* Summoner, const FTransform& SpawnTransform, const FVector Extent, const float ConeAngle, const float InnerRingPercent, const EFaction Hostility,
		const float DetonationTime, const bool bDestroyOnDetonate, const FLinearColor IndicatorColor, UTexture2D* IndicatorTexture, const float Intensity, const bool bAttach = false, USceneComponent* AttachComponent = nullptr,
		const FName SocketName = NAME_None);

//Helpers

public:

	UFUNCTION(BlueprintPure, Category = "Abilities")
	static FName GetRelevantTraceProfile(const AActor* Shooter, const bool bOverlap, const ESaiyoraPlane TracePlane, const EFaction TraceHostility);

	UFUNCTION(BlueprintPure, Category = "Line of Sight", meta = (HidePin = "Context", DefaultToSelf = "Context"))
	static bool CheckLineOfSightInPlane(const UObject* Context, const FVector& From, const FVector& To, const ESaiyoraPlane Plane);

private:
	
	static void GenerateOriginInfo(const ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin& OutOrigin);
	static void RewindRelevantHitboxesForShooter(const ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets,
		const TArray<AActor*>& ActorsToIgnore, TMap<UHitbox*, FTransform>& ReturnTransforms);
	static void RewindHitboxesToTimestamp(const TArray<UHitbox*>& Hitboxes, const float Timestamp, TMap<UHitbox*, FTransform>& ReturnTransforms);
	static void UnrewindHitboxes(const TMap<UHitbox*, FTransform>& ReturnTransforms);
	static float GetCameraTraceMaxRange(const FVector& CameraLoc, const FVector& AimDir, const FVector& OriginLoc, const float TraceRange);
	
	static void GetRelevantHitboxObjectTypes(const ASaiyoraPlayerCharacter* Shooter, const EFaction TraceHostility, TArray<TEnumAsByte<EObjectTypeQuery>>& OutObjectTypes);
	static void GetRelevantCollisionObjectTypes(const ESaiyoraPlane ProjectilePlane, TArray<TEnumAsByte<EObjectTypeQuery>>& OutObjectTypes);
};
