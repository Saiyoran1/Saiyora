#include "GroundAttack.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "CombatStatusComponent.h"
#include "CombatStructs.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Init

AGroundAttack::AGroundAttack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	
	if (IsValid(GetDecal()))
	{
		GetDecal()->DecalSize = FVector(100.0f);
		static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceObj(TEXT("MaterialInstanceConstant'/Game/Saiyora/Experiments/Decals/MI_GroundIndicator.MI_GroundIndicator'"));
		if (MaterialInstanceObj.Succeeded())
		{
			GetDecal()->SetDecalMaterial(MaterialInstanceObj.Object);
			DecalMaterial = GetDecal()->CreateDynamicMaterialInstance();
		}
		OuterBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, FName(TEXT("OuterBox")));
		if (IsValid(OuterBox))
		{
			OuterBox->SetBoxExtent(FVector(100.0f), true);
			OuterBox->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
			OuterBox->SetupAttachment(GetDecal());
		}
	}
}

void AGroundAttack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGroundAttack, ReplicatedDetonationParams);
	DOREPLIFETIME(AGroundAttack, VisualParams);
}

void AGroundAttack::ServerInit(const FGroundAttackVisualParams& InAttackParams, const FGroundAttackDetonationParams& InDetonationParams, const FGroundDetonationCallback& Callback)
{
	if (!HasAuthority() || bLocallyInitialized)
	{
		return;
	}
	VisualParams = InAttackParams;
	SetupVisuals();
	DetonationParams = InDetonationParams;
	if (GetOwner()->Implements<USaiyoraCombatInterface>())
	{
		const UCombatStatusComponent* OwnerCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
		if (IsValid(OwnerCombatStatus))
		{
			DetonationParams.AttackerFaction = OwnerCombatStatus->GetCurrentFaction();
			DetonationParams.AttackerPlane = OwnerCombatStatus->GetCurrentPlane();
		}
	}
	if (Callback.IsBound())
	{
		OnDetonation.Add(Callback);
	}
	SetupDetonation();
	bLocallyInitialized = true;
}

void AGroundAttack::PostNetReceive()
{
	Super::PostNetReceive();
	if (!bLocallyInitialized && VisualParams.bValid && ReplicatedDetonationParams.bValid)
	{
		SetupVisuals();
		SetupDetonation();
		bLocallyInitialized = true;
	}
}

#pragma endregion 
#pragma region Visuals

void AGroundAttack::SetupVisuals()
{
	if (!IsValid(DecalMaterial))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ground attack had no decal material when setting params!"));
		return;
	}
	
	//We don't set the actual decal extent because we also want to affect the hitbox of the effect.
	//Decal stays at the same "size" and is just scaled up.
	//We also flip Z and X here because decals like being 90 degree rotated for some reason.
	SetActorScale3D(FVector(VisualParams.DecalExtent.Z, VisualParams.DecalExtent.Y, VisualParams.DecalExtent.X) / 100.0f);
	//Cone angle doesn't actually change the hitbox, just visuals and how we filter hit targets during detonation.
	DecalMaterial->SetScalarParameterValue(FName(TEXT("ConeAngle")), VisualParams.ConeAngle);
	//Adjusting the inner ring doesn't actually change the hitbox, just visuals and how we filter hit targets during detonation.
	DecalMaterial->SetScalarParameterValue(FName(TEXT("InnerRadius")), VisualParams.InnerRingPercent);
	DecalMaterial->SetVectorParameterValue(FName(TEXT("Color")), VisualParams.IndicatorColor);
	DecalMaterial->SetTextureParameterValue(FName(TEXT("Texture")), VisualParams.IndicatorTexture);
	DecalMaterial->SetScalarParameterValue(FName(TEXT("Intensity")), VisualParams.IndicatorIntensity);

	//Server sets this flag to true so that clients can initialize when they receive the replicated params.
	if (HasAuthority())
	{
		VisualParams.bValid = true;
	}
}

#pragma endregion
#pragma region Detonation

void AGroundAttack::SetupDetonation()
{
	if (HasAuthority())
	{
		bool bBoundToCast = false;
		if (DetonationParams.bBindToCastEnd)
		{
			OwnerAbilityCompRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
			bBoundToCast = IsValid(OwnerAbilityCompRef) && OwnerAbilityCompRef->IsCasting();
		}
		if (bBoundToCast)
		{
			ReplicatedDetonationParams.bLooping = false;
			ReplicatedDetonationParams.bInfiniteLooping = false;
			ReplicatedDetonationParams.AdditionalLoops = 0;
			ReplicatedDetonationParams.DetonationInterval = -1.0f;
			ReplicatedDetonationParams.FirstDetonationTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + OwnerAbilityCompRef->GetCastTimeRemaining();
			
			OwnerAbilityCompRef->OnAbilityTick.AddDynamic(this, &AGroundAttack::DetonateOnCastFinalTick);
			OwnerAbilityCompRef->OnAbilityCancelled.AddDynamic(this, &AGroundAttack::CancelDetonationFromCastCancel);
			OwnerAbilityCompRef->OnAbilityInterrupted.AddDynamic(this, &AGroundAttack::CancelDetonationFromCastInterrupt);
		}
		else
		{
			ReplicatedDetonationParams.bLooping = DetonationParams.bLoopDetonation;
			ReplicatedDetonationParams.bInfiniteLooping = DetonationParams.bInfiniteLoop;
			ReplicatedDetonationParams.AdditionalLoops = DetonationParams.NumAdditionalLoops;
			ReplicatedDetonationParams.DetonationInterval = DetonationParams.DetonationDelay;
			ReplicatedDetonationParams.FirstDetonationTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + DetonationParams.DetonationDelay;
		}

		if (IsValid(OuterBox))
		{
			OuterBox->SetCollisionProfileName(UAbilityFunctionLibrary::GetRelevantProjectileHitboxProfile(DetonationParams.AttackerFaction, DetonationParams.FactionFilter));
		}
		
		//Set this flag so that clients can initialize when receiving the replicated params.
		ReplicatedDetonationParams.bValid = true;
	}
	StartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	LocalDetonationTime = ReplicatedDetonationParams.FirstDetonationTime;
	CompletedDetonationCounter = 0;
	SetActorTickEnabled(true);
}

void AGroundAttack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsValid(GetWorld()->GetGameState()))
	{
		return;
	}
	if (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() >= LocalDetonationTime)
	{
		DoDetonation();
	}
	else
	{
		if (IsValid(DecalMaterial))
		{
			const float Percent = FMath::Clamp((GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - StartTime) / (LocalDetonationTime - StartTime), 0.0f, 1.0f);
			DecalMaterial->SetScalarParameterValue(FName(TEXT("Percent")), Percent);
		}
	}
}

void AGroundAttack::DoDetonation()
{
	if (HasAuthority())
	{
		TArray<AActor*> HitActors;
		GetActorsInDetonation(HitActors);
		OnDetonation.Broadcast(this, HitActors);
	}
	CompletedDetonationCounter++;
	//If we aren't looping, or we're out of loops, we can go ahead and hide/destroy this attack.
	if (!ReplicatedDetonationParams.bLooping
		|| (!ReplicatedDetonationParams.bInfiniteLooping && CompletedDetonationCounter > ReplicatedDetonationParams.AdditionalLoops))
	{
		DestroyGroundAttack();
	}
	//If we are going to loop again, set up new timestamps for the next detonation.
	else
	{
		StartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		LocalDetonationTime = ReplicatedDetonationParams.FirstDetonationTime + CompletedDetonationCounter * ReplicatedDetonationParams.DetonationInterval;
	}
}

void AGroundAttack::DestroyGroundAttack()
{
	OnDetonation.Clear();
	if (HasAuthority())
	{
		if (IsValid(OwnerAbilityCompRef))
		{
			OwnerAbilityCompRef->OnAbilityTick.RemoveDynamic(this, &AGroundAttack::DetonateOnCastFinalTick);
			OwnerAbilityCompRef->OnAbilityCancelled.RemoveDynamic(this, &AGroundAttack::CancelDetonationFromCastCancel);
			OwnerAbilityCompRef->OnAbilityInterrupted.RemoveDynamic(this, &AGroundAttack::CancelDetonationFromCastInterrupt);
		}
		FTimerHandle DestroyHandle;
		GetWorld()->GetTimerManager().SetTimer(DestroyHandle, this, &AGroundAttack::DelayedDestroy, 1.0f);
		GetDecal()->DestroyComponent();
		SetActorTickEnabled(false);
		SetActorHiddenInGame(true);
		if (IsValid(OuterBox))
		{
			OuterBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		Destroy();
	}
}

void AGroundAttack::GetActorsInDetonation(TArray<AActor*>& HitActors) const
{
	HitActors.Empty();
	if (!IsValid(OuterBox))
	{
		return;
	}
	TArray<AActor*> Overlapping;
	OuterBox->GetOverlappingActors(Overlapping);
	UAbilityFunctionLibrary::FilterActorsByFaction(Overlapping, DetonationParams.AttackerFaction, DetonationParams.FactionFilter);
	UAbilityFunctionLibrary::FilterActorsByPlane(Overlapping, DetonationParams.AttackerPlane, DetonationParams.PlaneFilter);
	for (AActor* Actor : Overlapping)
	{
		const float AngleToActor = GetAngleToActor(Actor);
		const float RadiusAtAngle = GetRadiusAtAngle(AngleToActor);
		const float XYDistance = FVector::DistXY(Actor->GetActorLocation(), GetActorLocation());
		if (XYDistance > RadiusAtAngle)
		{
			continue;
		}
		if (VisualParams.InnerRingPercent > 0.0f)
		{
			if (XYDistance < RadiusAtAngle * (VisualParams.InnerRingPercent / 100.0f))
			{
				continue;
			}
		}
		if (FMath::Abs(AngleToActor) > (VisualParams.ConeAngle / 2))
		{
			continue;
		}
		HitActors.AddUnique(Actor);
	}
}

void AGroundAttack::DetonateOnCastFinalTick(const FAbilityEvent& Event)
{
	if (Event.Tick == Event.Ability->GetNonInitialTicks())
	{
		DoDetonation();
	}
}

float AGroundAttack::GetAngleToActor(AActor* Actor) const
{
	FVector Distance = Actor->GetActorLocation() - GetActorLocation();
	Distance.Z = 0.0f;
	Distance.Normalize();
	return UKismetMathLibrary::DegAcos(FVector::DotProduct(Distance, GetActorUpVector()));
}

float AGroundAttack::GetRadiusAtAngle(const float Angle) const
{
	const float Top = VisualParams.DecalExtent.X * VisualParams.DecalExtent.Y;
	const float XSquared = FMath::Square(VisualParams.DecalExtent.X * UKismetMathLibrary::DegSin(Angle));
	const float YSquared = FMath::Square(VisualParams.DecalExtent.Y * UKismetMathLibrary::DegCos(Angle));
	return (Top / (UKismetMathLibrary::Sqrt(XSquared + YSquared)));
}

#pragma endregion;