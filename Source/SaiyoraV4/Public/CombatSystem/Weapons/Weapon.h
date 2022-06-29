#pragma once
#include "CoreMinimal.h"
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

private:

	UPROPERTY()
	UWeaponComponent* Handler;
	bool bInitialized = false;
};
