#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "FireWeapon.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UFireWeapon : public UCombatAbility
{
	GENERATED_BODY()

	//Fire Delay

public:


private:

	UPROPERTY(EditDefaultsOnly, Category = "Fire Delay", meta = (ClampMin="0.05"))
	float DefaultFireDelay = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Fire Delay")
	bool bStaticFireDelay = false;
	UPROPERTY(EditDefaultsOnly, Category = "Fire Delay")
	bool bAutomatic = false;
	
	FTimerHandle FireDelayTimer;
	UFUNCTION()
	void EndFireDelay();

	//Firing

protected:

	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;

	//Weapon

protected:

	virtual void PreInitializeAbility_Implementation() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> WeaponClass;
	UPROPERTY()
	AWeapon* Weapon = nullptr;
};
