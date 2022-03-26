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
	UPROPERTY(NotReplicated)
	UCombatAbility* SourceAbility;
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
	void Replace();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Projectile")
	UCombatAbility* GetSource() const { return SourceInfo.SourceAbility; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Projectile")
	ASaiyoraPlayerCharacter* GetOwningPlayer() const { return SourceInfo.Owner; }
	
private:

	static int32 ProjectileID;
	static FPredictedTick PredictionScope;
	static int32 GenerateProjectileID(FPredictedTick const& Scope);

	UPROPERTY()
	class ASaiyoraGameState* GameState;
	UPROPERTY(EditAnywhere)
	UProjectileMovementComponent* ProjectileMovement;
	bool bIsFake = false;
	UPROPERTY(ReplicatedUsing=OnRep_SourceInfo)
	FProjectileSource SourceInfo;
	UFUNCTION()
	void OnRep_SourceInfo();
	FAbilityMispredictionCallback OnMisprediction;
	UFUNCTION()
	void DeleteOnMisprediction(int32 const PredictionID);
	bool bReplaced = false;
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