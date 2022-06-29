#include "Weapons/WeaponComponent.h"

#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "Weapons/Weapon.h"

UWeaponComponent::UWeaponComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponComponent, CurrentPrimaryWeapon);
	DOREPLIFETIME(UWeaponComponent, CurrentSecondaryWeapon);
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		SelectWeapon(true, DefaultPrimaryWeaponClass);
		if (bCanDualWield)
		{
			SelectWeapon(false, DefaultSecondaryWeaponClass);
		}
	}
}

void UWeaponComponent::SelectWeapon(const bool bPrimarySlot, const TSubclassOf<AWeapon> WeaponClass)
{
	if (!GetOwner()->HasAuthority() || !IsValid(WeaponClass) || (!bPrimarySlot && !bCanDualWield))
	{
		return;
	}
	AWeapon* PreviousWeapon = bPrimarySlot ? CurrentPrimaryWeapon : CurrentSecondaryWeapon;
	if (IsValid(PreviousWeapon))
	{
		PreviousWeapon->SwitchFromWeapon();
	}
	AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass);
	if (!IsValid(NewWeapon))
	{
		return;
	}
	NewWeapon->SetOwner(GetOwner());
	FName SocketName = NAME_None;
	USceneComponent* AttachComponent = bPrimarySlot ? ISaiyoraCombatInterface::Execute_GetPrimaryWeaponSocket(GetOwner(), SocketName) :
		ISaiyoraCombatInterface::Execute_GetSecondaryWeaponSocket(GetOwner(), SocketName);
	if (IsValid(AttachComponent))
	{
		const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, false);
		NewWeapon->AttachToComponent(AttachComponent, AttachRules, SocketName);
	}
	if (bPrimarySlot)
	{
		CurrentPrimaryWeapon = NewWeapon;
	}
	else
	{
		CurrentSecondaryWeapon = NewWeapon;
	}
	NewWeapon->InitWeapon(this);
	OnWeaponChanged.Broadcast(bPrimarySlot, PreviousWeapon, NewWeapon);
}

void UWeaponComponent::OnRep_CurrentPrimaryWeapon(AWeapon* PreviousWeapon)
{
	if (IsValid(PreviousWeapon))
	{
		PreviousWeapon->SwitchFromWeapon();
	}
	if (IsValid(CurrentPrimaryWeapon))
	{
		CurrentPrimaryWeapon->InitWeapon(this);
	}
	OnWeaponChanged.Broadcast(true, PreviousWeapon, CurrentPrimaryWeapon);
}

void UWeaponComponent::OnRep_CurrentSecondaryWeapon(AWeapon* PreviousWeapon)
{
	if (IsValid(PreviousWeapon))
	{
		PreviousWeapon->SwitchFromWeapon();
	}
	if (IsValid(CurrentSecondaryWeapon))
	{
		CurrentSecondaryWeapon->InitWeapon(this);
	}
	OnWeaponChanged.Broadcast(true, PreviousWeapon, CurrentSecondaryWeapon);
}
