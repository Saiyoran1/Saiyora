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
	ASaiyoraPlayerCharacter* Owner = nullptr;
	UPROPERTY()
	int32 ID = 0;
	UPROPERTY(NotReplicated)
	UCombatAbility* SourceAbility = nullptr;
};

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API APredictableProjectile : public AActor
{
	GENERATED_BODY()

public:
	
	FProjectileSource GetSourceInfo() const { return SourceInfo; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	APredictableProjectile(const class FObjectInitializer& ObjectInitializer);
	virtual void PostNetReceiveLocationAndRotation() override;
	void InitializeProjectile(UCombatAbility* Source);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile")
	bool IsFake() const { return bIsFake; }
	bool Replace();
	void UpdateLocallyDestroyed(bool const bLocallyDestroyed);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile")
	UCombatAbility* GetSource() const { return SourceInfo.SourceAbility; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile")
	ASaiyoraPlayerCharacter* GetOwningPlayer() const { return SourceInfo.Owner; }
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void DestroyProjectile();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile")
	bool IsDestroyed() const { return bDestroyed || bClientDestroyed; }

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnDestroy();
	
private:

	static int32 ProjectileID;
	static FPredictedTick PredictionScope;
	static int32 GenerateProjectileID(FPredictedTick const& Scope);

	UPROPERTY()
	class ASaiyoraGameState* GameState;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
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
	UPROPERTY(ReplicatedUsing=OnRep_Destroyed)
	bool bDestroyed = false;
	UFUNCTION()
	void OnRep_Destroyed();
	bool bClientDestroyed = false;
	FTimerHandle DestroyHandle;
	FTimerDelegate DestroyDelegate;
	UFUNCTION()
	void DelayedDestroy();
	void HideProjectile();
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