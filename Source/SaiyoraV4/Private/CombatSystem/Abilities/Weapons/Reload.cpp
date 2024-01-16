#include "Weapons/Reload.h"
#include "AbilityComponent.h"
#include "AmmoResource.h"
#include "ResourceHandler.h"
#include "SaiyoraPlayerCharacter.h"

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
	OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetHandler()->GetOwner());
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
	if (CastType == EAbilityCastType::Instant || TickNumber != GetNonInitialTicks())
	{
		StartReloadMontage();
	}
	if (GetHandler()->GetOwnerRole() != ROLE_Authority && CastType == EAbilityCastType::Instant || TickNumber == NonInitialTicks)
	{
		ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityDoubleReloadRestriction);
		bWaitingOnReloadResult = true;
	}
}

void UReload::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	if (IsValid(OwningPlayer) && !OwningPlayer->IsLocallyControlled() && (CastType == EAbilityCastType::Instant || TickNumber != GetNonInitialTicks()))
	{
		StartReloadMontage();
	}
}

void UReload::OnSimulatedTick_Implementation(const int32 TickNumber)
{
	Super::OnSimulatedTick_Implementation(TickNumber);
	if (IsValid(OwningPlayer) && !OwningPlayer->IsLocallyControlled() && (CastType == EAbilityCastType::Instant || TickNumber != GetNonInitialTicks()))
	{
		StartReloadMontage();
	}
}

void UReload::OnMisprediction_Implementation(const int32 PredictionID, const ECastFailReason FailReason)
{
	Super::OnMisprediction_Implementation(PredictionID, FailReason);
	CancelReloadMontage();
	if (bWaitingOnReloadResult)
	{
		bWaitingOnReloadResult = false;
		DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityDoubleReloadRestriction);
	}
}

void UReload::OnPredictedCancel_Implementation()
{
	Super::OnPredictedCancel_Implementation();
	CancelReloadMontage();
}

void UReload::OnServerCancel_Implementation()
{
	Super::OnServerCancel_Implementation();
	CancelReloadMontage();
}

void UReload::OnSimulatedCancel_Implementation()
{
	Super::OnSimulatedCancel_Implementation();
	CancelReloadMontage();
}

void UReload::StartReloadMontage()
{
	if (IsValid(ReloadMontage) && IsValid(OwningPlayer))
	{
		const float PlayRate = CastType == EAbilityCastType::Instant ? 1.0f : FMath::Max(ReloadMontage->GetPlayLength() / (GetHandler()->GetCurrentCastLength() / GetNonInitialTicks()), 0.01f);
		OwningPlayer->GetMesh()->GetAnimInstance()->Montage_Play(ReloadMontage, PlayRate);
	}
}

void UReload::CancelReloadMontage()
{
	if (IsValid(OwningPlayer) && OwningPlayer->GetMesh()->GetAnimInstance()->Montage_IsPlaying(ReloadMontage))
	{
		OwningPlayer->GetMesh()->GetAnimInstance()->Montage_Stop(0.0f, ReloadMontage);
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
