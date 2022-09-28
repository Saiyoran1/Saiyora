#include "CombatSystem/Abilities/Weapons/Weapon.h"

#include "AbilityComponent.h"
#include "FireWeapon.h"
#include "SaiyoraPlayerCharacter.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWeapon::FireWeapon()
{
	//TODO: Maybe use a fire delay threshold instead of a manual option, so that slowing down weapon fire results in using single shots.
	if (bFireSingleShots)
	{
		FireEffect();
	}
	else
	{
		if (bBurstFiring)
		{
			//Already firing, only play effect manually for local player. Sim players will auto-play effects on a timer.
			if (!IsValid(PlayerOwnerRef) || PlayerOwnerRef->IsLocallyControlled())
			{
				FireEffect();
			}
		}
		else
		{
			//Start firing!
			bBurstFiring = true;
			FireEffect();
			//Non-local players will re-fire regardless of receiving the next shot RPC to keep visuals consistent, until they are told to stop.
			if (IsValid(PlayerOwnerRef) && !PlayerOwnerRef->IsLocallyControlled() && IsValid(FireWeaponRef))
			{
				ThisBurstRefireDelay = FireWeaponRef->GetHandler()->CalculateCooldownLength(FireWeaponRef, true);
				if (ThisBurstRefireDelay > 0.0f)
				{
					GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AWeapon::RefireWeapon, ThisBurstRefireDelay);
				}
			}
		}
	}
}

void AWeapon::StopFiring()
{
	if (bBurstFiring)
	{
		bBurstFiring = false;
		GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
		StopFireEffect();
		ThisBurstRefireDelay = -1.0f;
	}
}

void AWeapon::RefireWeapon()
{
	if (bBurstFiring)
	{
		FireEffect();
		if (IsValid(PlayerOwnerRef) && !PlayerOwnerRef->IsLocallyControlled() && IsValid(FireWeaponRef))
		{
			//TODO: Currently this won't update visual effects if fire rate changes during a burst.
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AWeapon::RefireWeapon, ThisBurstRefireDelay);
		}
	}
}
