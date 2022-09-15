#include "FireWeapon.h"
#include "AbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "Weapon.h"

UFireWeapon::UFireWeapon()
{
	Plane = ESaiyoraPlane::Modern;
	bOnGlobalCooldown = false;
	DefaultChargeCost = 0;
	bStaticChargeCost = true;
	DefaultMaxCharges = 1;
	bStaticMaxCharges = true;
	CastType = EAbilityCastType::Instant;
	bAutomatic = true;
}

void UFireWeapon::EndFireDelay()
{
	DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
}

void UFireWeapon::OnPredictedTick_Implementation(const int32 TickNumber)
{
	Super::OnPredictedTick_Implementation(TickNumber);
	ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
	GetWorld()->GetTimerManager().SetTimer(FireDelayTimer, this, &UFireWeapon::EndFireDelay, GetHandler()->CalculateCooldownLength(this));
	if (IsValid(Weapon))
	{
		Weapon->StartFiring();
	}
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
			//Use actor root component if we don't have a designated weapon socket/component.
			WeaponSocketComp = GetHandler()->GetOwner()->GetRootComponent();
		}
		const FTransform WeaponTransform = WeaponSocketComp->GetSocketTransform(WeaponSocketName);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetHandler()->GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = Cast<AWeapon>(GetWorld()->SpawnActor(WeaponClass, &WeaponTransform, SpawnParams));
		if (IsValid(Weapon))
		{
			Weapon->AttachToComponent(WeaponSocketComp, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
		}
	}
	if (GetHandler()->GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(ReloadAbilityClass))
		{
			ReloadAbility = GetHandler()->AddNewAbility(ReloadAbilityClass);
		}
		//TODO: StopFiring ability, this might be able to be generic?
	}
}
