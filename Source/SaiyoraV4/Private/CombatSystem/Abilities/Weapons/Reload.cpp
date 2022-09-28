#include "Weapons/Reload.h"
#include "AbilityComponent.h"
#include "AmmoResource.h"
#include "ResourceHandler.h"

UReload::UReload()
{
	Plane = ESaiyoraPlane::Modern;
	MaxCharges.SetDefaultValue(1);
	MaxCharges.SetIsModifiable(false);
	ChargeCost.SetDefaultValue(0);
	ChargeCost.SetIsModifiable(false);
	AbilityTags = FGameplayTagContainer(FSaiyoraCombatTags::Get().ReloadAbility);
	bAutomatic = false;
	bOnGlobalCooldown = false;
}

void UReload::PreInitializeAbility_Implementation()
{
	Super::PreInitializeAbility_Implementation();
	if (GetHandler()->GetOwnerRole() != ROLE_SimulatedProxy && IsValid(GetHandler()->GetResourceHandlerRef()))
	{
		UResource* Ammo = GetHandler()->GetResourceHandlerRef()->FindActiveResource(UAmmoResource::StaticClass());
		if (IsValid(Ammo))
		{
			SetupAmmoTracking(Ammo);
		}
		else
		{
			GetHandler()->GetResourceHandlerRef()->OnResourceAdded.AddDynamic(this, &UReload::OnResourceAdded);
		}
	}
}

void UReload::OnPredictedTick_Implementation(const int32 TickNumber)
{
	Super::OnPredictedTick_Implementation(TickNumber);
	if (GetHandler()->GetOwnerRole() != ROLE_Authority && CastType == EAbilityCastType::Instant || TickNumber == NonInitialTicks)
	{
		ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityDoubleReloadRestriction);
		bWaitingOnReloadResult = true;
	}
}

void UReload::OnMisprediction_Implementation(const int32 PredictionID, const ECastFailReason FailReason)
{
	Super::OnMisprediction_Implementation(PredictionID, FailReason);
	if (bWaitingOnReloadResult)
	{
		bWaitingOnReloadResult = false;
		DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityDoubleReloadRestriction);
	}
}

void UReload::OnAmmoChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState)
{
	//On finish, restrict us from reloading immediately again until ammo changes.
	//TODO: Is there a scenario where the effects of the reload don't change the ammo, and will prevent ever reloading again?
	if (bWaitingOnReloadResult)
	{
		bWaitingOnReloadResult = false;
		DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityDoubleReloadRestriction);
	}
	if (bFullAmmo)
	{
		if (NewState.CurrentValue < NewState.Maximum)
		{
			bFullAmmo = false;
			DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFullReloadRestriction);
		}
	}
	else
	{
		if (NewState.CurrentValue >= NewState.Maximum)
		{
			bFullAmmo = true;
			ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFullReloadRestriction);
		}
	}
}

void UReload::OnResourceAdded(UResource* NewResource)
{
	if (IsValid(NewResource) && NewResource->GetClass() == UAmmoResource::StaticClass())
	{
		SetupAmmoTracking(NewResource);
		GetHandler()->GetResourceHandlerRef()->OnResourceAdded.RemoveDynamic(this, &UReload::OnResourceAdded);
	}
}

void UReload::SetupAmmoTracking(UResource* AmmoResource)
{
	AmmoRef = AmmoResource;
	AmmoRef->OnResourceChanged.AddDynamic(this, &UReload::OnAmmoChanged);
	if (AmmoRef->GetCurrentValue() >= AmmoRef->GetMaximum())
	{
		bFullAmmo = true;
		ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFullReloadRestriction);
	}
}
