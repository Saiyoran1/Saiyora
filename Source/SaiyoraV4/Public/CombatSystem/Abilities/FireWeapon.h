#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "FireWeapon.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UFireWeapon : public UCombatAbility
{
	GENERATED_BODY()

//Fire Delay
	
private:
	
	FTimerHandle FireDelayTimer;
	UFUNCTION()
	void EndFireDelay();

//Firing

protected:

	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;

//Ammo

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ammo")
	TSubclassOf<UCombatAbility> ReloadAbilityClass;
	UPROPERTY()
	UCombatAbility* ReloadAbility;
	
//Weapon

protected:

	virtual void PreInitializeAbility_Implementation() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> WeaponClass;
	UPROPERTY()
	AWeapon* Weapon = nullptr;
};
