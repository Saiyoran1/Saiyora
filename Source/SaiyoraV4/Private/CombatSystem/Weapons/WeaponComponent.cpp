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
	DOREPLIFETIME(UWeaponComponent, CurrentWeapon);
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		SelectWeapon(DefaultWeaponClass);
	}
}

void UWeaponComponent::SelectWeapon(const TSubclassOf<AWeapon> WeaponClass)
{
	if (!GetOwner()->HasAuthority() || !IsValid(WeaponClass))
	{
		return;
	}
	AWeapon* PreviousWeapon = CurrentWeapon;
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->SwitchFromWeapon();
	}
	AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	if (!IsValid(NewWeapon))
	{
		return;
	}
	FName SocketName = NAME_None;
	USceneComponent* AttachComponent = ISaiyoraCombatInterface::Execute_GetWeaponSocket(GetOwner(), SocketName);
	if (IsValid(AttachComponent))
	{
		const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, false);
		NewWeapon->AttachToComponent(AttachComponent, AttachRules, SocketName);
	}
	CurrentWeapon = NewWeapon;
	CurrentWeapon->InitWeapon(this);
	OnWeaponChanged.Broadcast(PreviousWeapon, CurrentWeapon);
}

void UWeaponComponent::OnRep_CurrentWeapon(AWeapon* PreviousWeapon)
{
	if (IsValid(PreviousWeapon))
	{
		PreviousWeapon->SwitchFromWeapon();
	}
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->InitWeapon(this);
	}
	OnWeaponChanged.Broadcast(PreviousWeapon, CurrentWeapon);
}
