#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UFireWeapon;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API AWeapon : public AActor
{
	GENERATED_BODY()

public:

	AWeapon();
	void FireWeapon() { if (!bFiring) { bFiring = true; StartFiring(); } }
	void StopFiringWeapon() { if (bFiring) { bFiring = false; StopFiring(); } }
	bool IsFiring() const { return bFiring; }

protected:
	
	UFUNCTION(BlueprintImplementableEvent)
	void StartFiring();
	UFUNCTION(BlueprintImplementableEvent)
	void StopFiring();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMeshComponent* WeaponMesh;

	bool bFiring = false;

};
