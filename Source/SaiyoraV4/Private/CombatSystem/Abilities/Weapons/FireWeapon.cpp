#include "Weapons/FireWeapon.h"
#include "AbilityComponent.h"
#include "AmmoResource.h"
#include "CombatStatusComponent.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"
#include "UnrealNetwork.h"
#include "Weapons/StopFiring.h"
#include "Weapons/Weapon.h"

UFireWeapon::UFireWeapon()
{
	Plane = ESaiyoraPlane::Modern;
	bOnGlobalCooldown = false;
	ChargeCost.SetDefaultValue(0);
	ChargeCost.SetIsModifiable(false);
	MaxCharges.SetDefaultValue(1);
	MaxCharges.SetIsModifiable(false);
	CastType = EAbilityCastType::Instant;
	bAutomatic = true;
}

void UFireWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UFireWeapon, AutoReloadState, COND_OwnerOnly);
}

void UFireWeapon::EndFireDelay()
{
	DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
}

void UFireWeapon::OnPredictedTick_Implementation(const int32 TickNumber)
{
	Super::OnPredictedTick_Implementation(TickNumber);
	ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
	GetWorld()->GetTimerManager().SetTimer(FireDelayTimer, this, &UFireWeapon::EndFireDelay, GetHandler()->CalculateCooldownLength(this, true));
	if (IsValid(Weapon))
	{
		Weapon->FireWeapon();
	}
}

void UFireWeapon::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	if (IsValid(Weapon))
	{
		Weapon->FireWeapon();
	}
}

void UFireWeapon::OnSimulatedTick_Implementation(const int32 TickNumber)
{
	Super::OnSimulatedTick_Implementation(TickNumber);
	if (IsValid(Weapon))
	{
		Weapon->FireWeapon();
	}
}

void UFireWeapon::PreInitializeAbility_Implementation()
{
	Super::PreInitializeAbility_Implementation();
	if (IsValid(WeaponClass))
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
			USaiyoraCombatLibrary::AttachCombatActorToComponent(Weapon, WeaponSocketComp, WeaponSocketName, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
			ASaiyoraPlayerCharacter* PlayerOwner = Cast<ASaiyoraPlayerCharacter>(GetHandler()->GetOwner());
			if (IsValid(PlayerOwner))
			{
				PlayerOwner->SetWeapon(Weapon);
			}
		}
	}
	if (GetHandler()->GetOwnerRole() == ROLE_Authority)
	{
		if (bAutoReloadXPlane)
		{
			ResourceHandlerRef = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetHandler()->GetOwner());
			if (IsValid(ResourceHandlerRef))
			{
				ResourceHandlerRef->AddNewResource(UAmmoResource::StaticClass(), AmmoInitInfo);
				AmmoResourceRef = ResourceHandlerRef->FindActiveResource(UAmmoResource::StaticClass());
				if (IsValid(AmmoResourceRef))
				{
					UCombatStatusComponent* OwnerCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetHandler()->GetOwner());
					if (IsValid(OwnerCombatStatus))
					{
						OwnerCombatStatus->OnPlaneSwapped.AddDynamic(this, &UFireWeapon::StartAutoReloadOnPlaneSwap);
					}
				}
			}
		}
		if (IsValid(ReloadAbilityClass))
		{
			GetHandler()->AddNewAbility(ReloadAbilityClass);
		}
		GetHandler()->AddNewAbility(UStopFiring::StaticClass());
	}
}

void UFireWeapon::StartAutoReloadOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane,
	UObject* Source)
{
	if (NewPlane != GetAbilityPlane())
	{
		if (IsValid(AmmoResourceRef) && AmmoResourceRef->GetCurrentValue() < AmmoResourceRef->GetMaximum() && !GetWorld()->GetTimerManager().IsTimerActive(AutoReloadHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(AutoReloadHandle, this, &UFireWeapon::FinishAutoReload, AutoReloadTime);
			AutoReloadState.bIsAutoReloading = true;
			AutoReloadState.StartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
			AutoReloadState.EndTime = AutoReloadState.StartTime + AutoReloadTime;
		}
	}
	else if (GetWorld()->GetTimerManager().IsTimerActive(AutoReloadHandle))
	{
		
		GetWorld()->GetTimerManager().ClearTimer(AutoReloadHandle);
		AutoReloadState.bIsAutoReloading = false;
		AutoReloadState.StartTime = 0.0f;
		AutoReloadState.EndTime = 0.0f;
	}
}

void UFireWeapon::FinishAutoReload()
{
	if (IsValid(AmmoResourceRef))
	{
		AmmoResourceRef->ModifyResource(this, (AmmoResourceRef->GetMaximum() - AmmoResourceRef->GetCurrentValue()), true);
	}
	AutoReloadState.bIsAutoReloading = false;
	AutoReloadState.StartTime = 0.0f;
	AutoReloadState.EndTime = 0.0f;
}