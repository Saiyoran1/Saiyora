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

	FORCEINLINE bool operator==(FProjectileSource const& Other) const
	{
		return Other.SourceClass == SourceClass && Other.SourceTick == SourceTick && Other.Owner == Owner && Other.ID == ID;
	}
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
	APredictableProjectile* FindMatchingFakeProjectile() const;
	bool Matches(APredictableProjectile const* Other) const { return Other->GetSourceInfo() == SourceInfo; }
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