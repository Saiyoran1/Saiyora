#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API AWeapon : public AActor
{
	GENERATED_BODY()

public:

	AWeapon();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMeshComponent* WeaponMesh;
};
