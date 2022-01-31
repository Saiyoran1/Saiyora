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

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API APredictableProjectile : public AActor
{
	GENERATED_BODY()

public:
	
	FProjectileSource GetSourceInfo() const { return SourceInfo; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	APredictableProjectile(const class FObjectInitializer& ObjectInitializer);
	int32 InitializeClient(UCombatAbility* Source);
	void InitializeServer(UCombatAbility* Source, int32 const ClientID, float const PingComp);
	bool IsFake() const { return bIsFake; }
	bool HasPredictedHit() const { return PredictedHit.Key; }
	
private:

	static int32 ProjectileID;

	int32 GenerateProjectileID();

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