#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UFireWeapon;
class ASaiyoraPlayerCharacter;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API AWeapon : public AActor
{
	GENERATED_BODY()

public:

	AWeapon();
	void Initialize(ASaiyoraPlayerCharacter* NewPlayerOwner, UFireWeapon* FireWeaponAbility) { PlayerOwnerRef = NewPlayerOwner; FireWeaponRef = FireWeaponAbility; }
	void FireWeapon();
	void StopFiring();

	bool IsBurstFiring() const { return bBurstFiring; }

protected:
	
	UFUNCTION(BlueprintImplementableEvent)
	void FireEffect();
	UFUNCTION(BlueprintImplementableEvent)
	void StopFireEffect();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMeshComponent* WeaponMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bFireSingleShots = true;

	bool bBurstFiring = false;
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerOwnerRef;
	UPROPERTY()
	UFireWeapon* FireWeaponRef;

	FTimerHandle RefireTimer;
	float ThisBurstRefireDelay = -1.0f;
	UFUNCTION()
	void RefireWeapon();
};
