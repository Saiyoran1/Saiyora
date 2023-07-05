#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PredictableProjectile.generated.h"

class USaiyoraProjectileComponent;
class ASaiyoraPlayerCharacter;
class UCombatAbility;

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
	virtual void PostNetInit() override;
	void InitializeProjectile(UCombatAbility* Source, const FPredictedTick& Tick, const int32 ID, const ESaiyoraPlane ProjectilePlane, const EFaction ProjectileHostility);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile")
	bool IsFake() const { return bIsFake; }
	bool Replace();
	void UpdateLocallyDestroyed(bool const bLocallyDestroyed);
	void MarkCatchupComplete() { bShouldReplace = true; SetReplicateMovement(false); }

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USaiyoraProjectileComponent* ProjectileMovement;
	bool bIsFake = false;
	UPROPERTY(ReplicatedUsing=OnRep_SourceInfo)
	FProjectileSource SourceInfo;
	UFUNCTION()
	void OnRep_SourceInfo();
	UFUNCTION()
	void DeleteOnMisprediction(int32 const PredictionID);
	UPROPERTY(ReplicatedUsing=OnRep_ShouldReplace)
	bool bShouldReplace = false;
	UFUNCTION()
	void OnRep_ShouldReplace();
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
	UPROPERTY()
	TMap<UPrimitiveComponent*, FName> PreHideCollision;
};