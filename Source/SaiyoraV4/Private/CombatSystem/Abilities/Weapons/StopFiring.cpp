#include "Weapons/StopFiring.h"
#include "AbilityComponent.h"
#include "SaiyoraPlayerCharacter.h"
#include "Weapons/FireWeapon.h"
#include "Weapons/Weapon.h"

UStopFiring::UStopFiring()
{
	Plane = ESaiyoraPlane::Modern;
	ChargeCost.SetDefaultValue(0);
	ChargeCost.SetIsModifiable(false);
	MaxCharges.SetDefaultValue(1);
	MaxCharges.SetIsModifiable(false);
	bOnGlobalCooldown = false;
	CastType = EAbilityCastType::Instant;
	bAutomatic = false;
	AbilityTags = FGameplayTagContainer(FSaiyoraCombatTags::Get().StopFireAbility);
}

void UStopFiring::PreInitializeAbility_Implementation()
{
	Super::PreInitializeAbility_Implementation();
	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetHandler()->GetOwner());
}

void UStopFiring::OnPredictedTick_Implementation(const int32 TickNumber)
{
	Super::OnPredictedTick_Implementation(TickNumber);
	StopFiringWeapon();
}

void UStopFiring::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	StopFiringWeapon();
}

void UStopFiring::OnSimulatedTick_Implementation(const int32 TickNumber)
{
	Super::OnSimulatedTick_Implementation(TickNumber);
	StopFiringWeapon();
}

void UStopFiring::StopFiringWeapon()
{
	if (IsValid(OwningPlayer) && IsValid(OwningPlayer->GetWeapon()))
	{
		OwningPlayer->GetWeapon()->StopFiringWeapon();
	}
}