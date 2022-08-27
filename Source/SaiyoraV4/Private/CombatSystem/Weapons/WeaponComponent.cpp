#include "Weapons/WeaponComponent.h"

#include "AbilityComponent.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "PlaneComponent.h"
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

FWeaponFireResult UWeaponComponent::FireWeapon(const bool bPrimary)
{
	const bool bFromQueue = bFiringWeaponFromQueue;
	bFiringWeaponFromQueue = false;
	//TODO: Expire queue?
	FWeaponFireResult Result;
	if (!IsValid(OwnerAsPawn) || !OwnerAsPawn->IsLocallyControlled())
	{
		Result.FailReason = EWeaponFailReason::NetRole;
		return Result;
	}
	AWeapon* Weapon = bPrimary ? CurrentPrimaryWeapon : CurrentSecondaryWeapon;
	if (!CanFireWeapon(Weapon, Result.FailReason))
	{
		if (!bFromQueue && Result.FailReason == EWeaponFailReason::Casting)
		{
			if (TryQueueWeapon(bPrimary))
			{
				Result.FailReason = EWeaponFailReason::Queued;
				return Result;
			}
			//If we can't queue (the cast has too long remaining), cancel the current cast and re-check CanFireWeapon.
			//If the cast can't be cancelled OR CanFireWeapon fails again, just return.
			if (!AbilityCompRef->CancelCurrentCast().bSuccess || !CanFireWeapon(Weapon, Result.FailReason))
			{
				return Result;
			}
		}
		else
		{
			return Result;
		}
	}
	//TODO: Generate prediction ID. Ideally need a place central that both ability and weapon can access.
	//TODO: Store info client-side.
	//TODO: RPC shot to server.
	//TODO: Call weapon firing.
	return Result;
}

bool UWeaponComponent::CanFireWeapon(AWeapon* Weapon, EWeaponFailReason& OutFailReason) const
{
	OutFailReason = EWeaponFailReason::None;
	if (!IsValid(Weapon))
	{
		OutFailReason = EWeaponFailReason::InvalidWeapon;
		return false;
	}
	if (IsValid(AbilityCompRef) && AbilityCompRef->IsCasting())
	{
		OutFailReason = EWeaponFailReason::Casting;
		return false;
	}
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		OutFailReason = EWeaponFailReason::Dead;
		return false;
	}
	if (IsValid(PlaneComponentRef) && PlaneComponentRef->GetCurrentPlane() != ESaiyoraPlane::Modern && PlaneComponentRef->GetCurrentPlane() != ESaiyoraPlane::Both)
	{
		OutFailReason = EWeaponFailReason::WrongPlane;
		return false;
	}
	if (IsValid(CcHandlerRef) && (CcHandlerRef->GetCrowdControlStatus(FSaiyoraCombatTags::Get().Cc_Stun).bActive
		|| CcHandlerRef->GetCrowdControlStatus(FSaiyoraCombatTags::Get().Cc_Incapacitate).bActive
		|| CcHandlerRef->GetCrowdControlStatus(FSaiyoraCombatTags::Get().Cc_Disarm).bActive))
	{
		OutFailReason = EWeaponFailReason::CrowdControl;
		return false;
	}
	//TODO: Custom conditions
	//TODO: Weapon-specific (ammo, fire rate, custom requirements)
	return true;
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
