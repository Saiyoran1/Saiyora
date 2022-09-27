#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "FireWeapon.generated.h"

class AWeapon;

USTRUCT(BlueprintType)
struct FAutoReloadState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsAutoReloading = false;
	UPROPERTY()
	float StartTime = 0.0f;
	UPROPERTY()
	float EndTime = 0.0f;
};

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UFireWeapon : public UCombatAbility
{
	GENERATED_BODY()

public:

	UFireWeapon();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

//Fire Delay
	
private:
	
	FTimerHandle FireDelayTimer;
	UFUNCTION()
	void EndFireDelay();

//Firing

protected:

	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void OnSimulatedTick_Implementation(const int32 TickNumber) override;

//Ammo

public:

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAutoReloading() const { return AutoReloadState.bIsAutoReloading; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadStartTime() const { return AutoReloadState.StartTime; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadEndTime() const { return AutoReloadState.EndTime; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadTimeRemaining() const { return AutoReloadState.bIsAutoReloading ? FMath::Max(0.0f, AutoReloadState.EndTime - GetWorld()->GetGameState()->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadTime() const { return AutoReloadTime; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCombatAbility> ReloadAbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bAutoReloadXPlane = true;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float AutoReloadTime = 5.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FResourceInitInfo AmmoInitInfo;
	
	UPROPERTY(Replicated)
	FAutoReloadState AutoReloadState;
	FTimerHandle AutoReloadHandle;
	UFUNCTION()
	void StartAutoReloadOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
	UFUNCTION()
	void FinishAutoReload();

	UPROPERTY()
	UResourceHandler* ResourceHandlerRef;
	UPROPERTY()
	UResource* AmmoResourceRef;
	
//Weapon
	
public:

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetWeapon() const { return Weapon; }

protected:

	virtual void PreInitializeAbility_Implementation() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeapon> WeaponClass;
	
	UPROPERTY()
	AWeapon* Weapon = nullptr;
};
