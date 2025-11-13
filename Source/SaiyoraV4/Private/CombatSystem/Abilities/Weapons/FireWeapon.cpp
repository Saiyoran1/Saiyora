#include "Weapons/FireWeapon.h"
#include "AbilityComponent.h"
#include "AmmoResource.h"
#include "CombatStatusComponent.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapons/Reload.h"
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

float UFireWeapon::GetCooldownLength()
{
	if (bStaticCooldownLength)
	{
		return DefaultCooldownLength;
	}
	return GetHandler()->CalculateCooldownLength(this, true);
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
	UpdateSpreadForShot();
}

void UFireWeapon::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	if (IsValid(Weapon))
	{
		Weapon->FireWeapon();
	}
	AddNewShotForVerification(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}

void UFireWeapon::OnSimulatedTick_Implementation(const int32 TickNumber)
{
	Super::OnSimulatedTick_Implementation(TickNumber);
	if (IsValid(Weapon))
	{
		Weapon->FireWeapon();
	}
}

void UFireWeapon::PostInitializeAbility_Implementation()
{
	Super::PostInitializeAbility_Implementation();
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
				Weapon->Initialize(PlayerOwner, this);
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

					//Set verification params based on an expected number of shots per clip.
					TArray<FSimpleAbilityCost> Costs;
					GetAbilityCosts(Costs);
					int32 ExpectedClipShots = MinShotsToVerify;
					for (const FSimpleAbilityCost& Cost : Costs)
					{
						if (Cost.ResourceClass == UAmmoResource::StaticClass() && Cost.Cost > 0.0f)
						{
							ExpectedClipShots = FMath::Floor((AmmoResourceRef->GetMaximum() / Cost.Cost) / 2.0f);
						}
					}
					//MinShotsToVerify is the number of shots that have to occur before we start verifying.
					//This needs to be at least 2 obviously, but also we want to verify at least once per clip, so verifying at least every half clip is a good rule of thumb.
					MinShotsToVerify = FMath::Clamp(MinShotsToVerify, 2, ExpectedClipShots / 2);
					//MaxShotsToVerify is the number of shots we keep at a time to verify against.
					//This needs to be at least the number of shots needed to trigger verification, but there's no reason to keep more than a clip's worth of shots.
					MaxShotsToVerify = FMath::Clamp(MaxShotsToVerify, MinShotsToVerify, ExpectedClipShots);
				}
			}
		}
		if (IsValid(ReloadAbilityClass))
		{
			GetHandler()->AddNewAbility(ReloadAbilityClass);
			GetHandler()->OnAbilityTick.AddDynamic(this, &UFireWeapon::OnReloadComplete);
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

	ResetVerificationWindow();
}


void UFireWeapon::AddNewShotForVerification(const float Timestamp)
{
	CurrentClipTimestamps.Add(FShotVerificationInfo(Timestamp, GetHandler()->CalculateCooldownLength(this, true)));
	while (CurrentClipTimestamps.Num() > MaxShotsToVerify)
	{
		CurrentClipTimestamps.RemoveAt(0);
	}
	if (CurrentClipTimestamps.Num() >= MinShotsToVerify)
	{
		VerifyFireRate();
	}
}

void UFireWeapon::VerifyFireRate()
{
	const float TotalTime = CurrentClipTimestamps.Last().Timestamp - CurrentClipTimestamps[0].Timestamp;
	float TotalCooldownTime = 0.0f;
	for (int32 i = 0; i < CurrentClipTimestamps.Num() - 1; i++)
	{
		TotalCooldownTime += CurrentClipTimestamps[i].CooldownAtTime;
	}
	if (TotalTime < TotalCooldownTime * (1 - FIRERATEERRORMARGIN))
	{
		ActivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction);
		const float TimeUntilNextValidShot = (TotalCooldownTime - TotalTime) + GetHandler()->CalculateCooldownLength(this, true);
		GetWorld()->GetTimerManager().SetTimer(ServerFireRateTimer, this, &UFireWeapon::EndFireRateVerificationWait, TimeUntilNextValidShot);
	}
}

void UFireWeapon::OnReloadComplete(const FAbilityEvent& AbilityEvent)
{
	if (AbilityEvent.ActionTaken == ECastAction::Tick && AbilityEvent.Ability->GetClass() == ReloadAbilityClass && AbilityEvent.Tick == AbilityEvent.Ability->GetNonInitialTicks())
	{
		ResetVerificationWindow();
	}
}

#pragma region Crosshair

void UFireWeapon::UpdateSpreadForShot()
{
	//If this weapon doesn't have a spread curve, it just doesn't have spread.
	if (!IsValid(SpreadCurve))
	{
		return;
	}
	//Update our delay time until spread starts to decay. This is reset each time we shoot.
	if (SpreadDecayDelay > 0.0f)
	{
		SpreadDecayDelayRemaining = SpreadDecayDelay;
	}
	//If our spread was in the process of decaying, stop decaying.
	if (bDecayingSpread)
	{
		EndSpreadDecay();
	}
	//Update spread alpha (0-1) which is what we use to access the angle from our curve.
	CurrentSpreadAlpha = FMath::Min(CurrentSpreadAlpha + SpreadAlphaPerShot, 1.0f);
	//Update the spread angle from our curve using the new alpha.
	const float PreviousAngle = CurrentSpreadAngle;
	CurrentSpreadAngle = SpreadCurve->GetFloatValue(CurrentSpreadAlpha);
	if (PreviousAngle != CurrentSpreadAngle)
	{
		OnSpreadChanged.Broadcast(CurrentSpreadAngle);
	}
}

void UFireWeapon::StartSpreadDecay()
{
	bDecayingSpread = true;
	DecayStartAlpha = CurrentSpreadAlpha;
	DecayStartAngle = CurrentSpreadAngle;
	CurrentDecayLength = SpreadDecayLength * CurrentSpreadAlpha;
	TimeDecaying = 0.0f;
}

void UFireWeapon::EndSpreadDecay()
{
	bDecayingSpread = false;
	DecayStartAlpha = 0.0f;
	DecayStartAngle = 0.0f;
	CurrentDecayLength = 0.0f;
	TimeDecaying = 0.0f;
}

void UFireWeapon::TickSpreadDecay(const float DeltaTime)
{
	const ASaiyoraPlayerCharacter* OwningPlayer = Cast<ASaiyoraPlayerCharacter>(GetHandler()->GetOwner());
	if (IsValid(OwningPlayer))
	{
		const FVector Forward = OwningPlayer->Camera->GetForwardVector();
		DrawDebugLine(OwningPlayer->GetWorld(), OwningPlayer->Camera->GetComponentLocation() + Forward * 100.0f,
			OwningPlayer->Camera->GetComponentLocation() + Forward * 1000.0f, FColor::Blue);
		DrawDebugSphere(OwningPlayer->GetWorld(), OwningPlayer->Camera->GetComponentLocation() + Forward * 1000.f,
			UKismetMathLibrary::DegTan(CurrentSpreadAngle) * 1000.0f, 32, FColor::Blue);
	}
	
	//If we have no spread, then no need to decay.
	if (CurrentSpreadAlpha <= 0.0f && CurrentSpreadAngle <= 0.0f)
	{
		return;
	}
	//If we recently shot, we might be waiting to decay the spread value.
	float ActualDelta = DeltaTime;
	if (SpreadDecayDelayRemaining > 0.0f)
	{
		//Reduce the decay delay time.
		SpreadDecayDelayRemaining -= DeltaTime;
		//If we should still be waiting after this tick, then return.
		if (SpreadDecayDelayRemaining > 0.0f)
		{
			return;
		}
		//Check how much extra time we have beyond finishing our delay.
		ActualDelta = FMath::Abs(SpreadDecayDelayRemaining);
		SpreadDecayDelayRemaining = 0.0f;
	}
	//If we just started decaying our spread, set up some initial values.
	//We don't just go backward over the curve, as we want spread decay to be linear.
	//We also need to decay the alpha value under the hood separately,
	//since there isn't a good way to invert the curve or handle situations where alpha might be > 0 even though spread is still 0.
	if (!bDecayingSpread)
	{
		StartSpreadDecay();
	}
	//We'll use TimeDecaying to calculate an alpha value to lerp from our decay start to 0 for both spread alpha and angle.
	TimeDecaying = FMath::Min(TimeDecaying + ActualDelta, CurrentDecayLength);
	const float PreviousSpread = CurrentSpreadAngle;
	//Once we've reached the total length of the spread decay, everything should ultimately be set to 0 and decay should be turned off.
	if (CurrentDecayLength <= 0.0f || TimeDecaying >= CurrentDecayLength)
	{
		EndSpreadDecay();
		CurrentSpreadAngle = 0.0f;
		CurrentSpreadAlpha = 0.0f;
	}
	else
	{
		//If we haven't finished decaying, we'll just lerp both alpha and angle to the correct values.
		const float DecayAlpha = FMath::Clamp(TimeDecaying / CurrentDecayLength, 0.0f, 1.0f);
		CurrentSpreadAlpha = FMath::Lerp(DecayStartAlpha, 0.0f, DecayAlpha);
		CurrentSpreadAngle = FMath::Lerp(DecayStartAngle, 0.0f, DecayAlpha);
	}
	if (PreviousSpread != CurrentSpreadAngle)
	{
		OnSpreadChanged.Broadcast(CurrentSpreadAngle);
	}
}

bool UFireWeapon::IsTickable() const
{
	return IsValid(GetHandler()) && (IsValid(SpreadCurve) || IsValid(RecoilCurve));
}

#pragma endregion