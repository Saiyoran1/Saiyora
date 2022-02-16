#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CombatAbility.h"
#include "PredictableProjectile.generated.h"

USTRUCT()
struct FProjectileSource
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UCombatAbility> SourceClass;
	UPROPERTY()
	FPredictedTick SourceTick;
	UPROPERTY()
	ASaiyoraPlayerCharacter* Owner;
	UPROPERTY()
	int32 ID;
};

UENUM()
enum class EProjectileHostility : uint8
{
	None,
	Player,
	Enemy,
	Both
};

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API APredictableProjectile : public AActor
{
	GENERATED_BODY()

public:
	
	FProjectileSource GetSourceInfo() const { return SourceInfo; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	APredictableProjectile(const class FObjectInitializer& ObjectInitializer);
	void InitializeProjectile(UCombatAbility* Source);
	bool IsFake() const { return bIsFake; }
	bool HasPredictedHit() const { return PredictedHit.Key; }
	void Replace();
	
private:

	static int32 ProjectileID;
	static FPredictedTick PredictionScope;
	int32 GenerateProjectileID(FPredictedTick const& Scope);

	UPROPERTY(EditAnywhere)
	UProjectileMovementComponent* ProjectileMovement;
	bool bIsFake = false;
	UPROPERTY(ReplicatedUsing=OnRep_SourceInfo)
	FProjectileSource SourceInfo;
	UFUNCTION()
	void OnRep_SourceInfo();
	UPROPERTY(ReplicatedUsing=OnRep_FinalHit)
	FHitResult FinalHit;
	UFUNCTION()
	void OnRep_FinalHit();
	TTuple<bool, FHitResult> PredictedHit;
	FAbilityMispredictionCallback OnMisprediction;
	UFUNCTION()
	void DeleteOnMisprediction(int32 const PredictionID);
};

USTRUCT()
struct FProjectileSpawnParams
{
	GENERATED_BODY()
	
	UPROPERTY()
	TSubclassOf<APredictableProjectile> Class;
	UPROPERTY()
	FTransform SpawnTransform;
};