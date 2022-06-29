#include "Weapons/Weapon.h"

AWeapon::AWeapon()
{
	SetReplicates(true);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AWeapon::InitWeapon(UWeaponComponent* NewHandler)
{
	if (bInitialized)
	{
		return;
	}
	Handler = NewHandler;
	//TODO: Blueprint native event call to handle weapon init?
}

void AWeapon::SwitchFromWeapon()
{
	//TODO: Blueprint native event call to handle weapon cleanup?
}


