#include "Weapons/Weapon.h"
#include "UnrealNetwork.h"

AWeapon::AWeapon()
{
	SetReplicates(true);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, MaxAmmo, COND_OwnerOnly);
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

void AWeapon::CommitAmmo(const int32 AmmoCost)
{
	//TODO: Add some way to check if ammo has already been committed for this shot?
	//Client prediction won't work correctly if this is called multiple times with the same shot ID.
	if (AmmoCost < 0 || !bIsFiring)
	{
		return;
	}
	if (GetLocalRole() == ROLE_Authority)
	{
		const int32 PreviousAmmo = CurrentAmmo.CurrentAmmo;
		CurrentAmmo.CurrentAmmo = FMath::Clamp(CurrentAmmo.CurrentAmmo - AmmoCost, 0, MaxAmmo);
		CurrentAmmo.ShotID = ShotID;
		if (CurrentAmmo.CurrentAmmo != PreviousAmmo)
		{
			OnAmmoChanged.Broadcast(PreviousAmmo, CurrentAmmo.CurrentAmmo);
		}
	}
	else if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		const int32 PreviousAmmo = ClientAmmo;
		PredictedAmmoCosts.Add(ShotID, AmmoCost);
		CalculateNewPredictedAmmoValue();
		if (PreviousAmmo != ClientAmmo)
		{
			OnAmmoChanged.Broadcast(PreviousAmmo, ClientAmmo);
		}
	}
}

void AWeapon::OnRep_MaxAmmo(const int32 PreviousMaxAmmo)
{
	OnMaxAmmoChanged.Broadcast(PreviousMaxAmmo, MaxAmmo);
}

void AWeapon::OnRep_CurrentAmmo()
{
	PurgeOldAmmoPredictions();
	CalculateNewPredictedAmmoValue();
}

void AWeapon::PurgeOldAmmoPredictions()
{
	TArray<int32> IDs;
	PredictedAmmoCosts.GetKeys(IDs);
	for (const int32 ID : IDs)
	{
		if (ID <= CurrentAmmo.ShotID)
		{
			PredictedAmmoCosts.Remove(ID);
		}
	}
}

void AWeapon::CalculateNewPredictedAmmoValue()
{
	const int32 PreviousAmmo = ClientAmmo;
	int32 NewAmmo = CurrentAmmo.CurrentAmmo;
	for (const TTuple<int32, int32>& Prediction : PredictedAmmoCosts)
	{
		NewAmmo += Prediction.Value;
	}
	ClientAmmo = FMath::Clamp(NewAmmo, 0, DefaultMaxAmmo);
	if (ClientAmmo != PreviousAmmo)
	{
		OnAmmoChanged.Broadcast(PreviousAmmo, ClientAmmo);
	}
}


