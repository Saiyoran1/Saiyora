#pragma once
#include "CoreMinimal.h"
#include "WeaponStructs.h"
#include "Weapon.generated.h"

class UWeaponComponent;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API AWeapon : public AActor
{
	GENERATED_BODY()

public:

	AWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitWeapon(UWeaponComponent* NewHandler);
	void SwitchFromWeapon();

	void StartFiring() {/*TODO*/}
	void EndFiring() {/*TODO*/}

	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnAmmoChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnMaxAmmoChanged;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return GetLocalRole() == ROLE_AutonomousProxy ? ClientAmmo : CurrentAmmo.CurrentAmmo; }

protected:

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void CommitAmmo(const int32 AmmoCost = 1);

private:

	UPROPERTY()
	UWeaponComponent* Handler;
	bool bInitialized = false;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bAutomatic = false;
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = ".01"))
	float TimeBetweenAutoShots = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bInfiniteAmmo = false;
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1"))
	int32 DefaultMaxAmmo = 1;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bStaticMaxAmmo = false;
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0"))
	int32 DefaultAmmoCost = 1;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bStaticAmmoCost = false;

	bool bIsFiring = false;
	int32 ShotID = 0;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxAmmo)
	int32 MaxAmmo = 1;
	UFUNCTION()
	void OnRep_MaxAmmo(const int32 PreviousMaxAmmo);
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo)
	FAmmo CurrentAmmo;
	UFUNCTION()
	void OnRep_CurrentAmmo();
	int32 ClientAmmo;
	TMap<int32, int32> PredictedAmmoCosts;
	void PurgeOldAmmoPredictions();
	void CalculateNewPredictedAmmoValue();
};
