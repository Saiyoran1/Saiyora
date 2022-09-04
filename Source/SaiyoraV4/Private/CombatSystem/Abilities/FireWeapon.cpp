#include "FireWeapon.h"

#include "AbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "Weapon.h"

void UFireWeapon::EndFireDelay()
{
	DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
	if (bAutomatic)
	{
		//TODO: Check if fire input is down? Then trigger re-fire?
		//Or I could just set up a universal input system for abilities that includes cancel on release, re-fire on hold, etc.
	}
}

void UFireWeapon::OnPredictedTick_Implementation(const int32 TickNumber)
{
	Super::OnPredictedTick_Implementation(TickNumber);
	ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
	GetWorld()->GetTimerManager().SetTimer(FireDelayTimer, this, &UFireWeapon::EndFireDelay, DefaultFireDelay);
}

void UFireWeapon::PreInitializeAbility_Implementation()
{
	Super::PreInitializeAbility_Implementation();
	if (IsValid(WeaponClass) && GetHandler()->GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		FName WeaponSocketName = NAME_None;
		USceneComponent* WeaponSocketComp = ISaiyoraCombatInterface::Execute_GetPrimaryWeaponSocket(GetHandler()->GetOwner(), WeaponSocketName);
		if (!IsValid(WeaponSocketComp))
		{
			return;
		}
		const FTransform WeaponTransform = WeaponSocketComp->GetSocketTransform(WeaponSocketName);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetHandler()->GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = Cast<AWeapon>(GetWorld()->SpawnActor(WeaponClass, &WeaponTransform, SpawnParams));
		Weapon->AttachToComponent(WeaponSocketComp, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
	}
}
