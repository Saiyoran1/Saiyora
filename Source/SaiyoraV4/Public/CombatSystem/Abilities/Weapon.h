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
	UFUNCTION(BlueprintImplementableEvent)
	void StartFiring();
	UFUNCTION(BlueprintImplementableEvent)
	void StopFiring();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMeshComponent* WeaponMesh;

};
