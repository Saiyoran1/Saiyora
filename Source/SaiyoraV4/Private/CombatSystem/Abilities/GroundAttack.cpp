#include "GroundAttack.h"
#include "CombatStatusComponent.h"
#include "CombatStructs.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetMathLibrary.h"

AGroundAttack::AGroundAttack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
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
	DOREPLIFETIME(AGroundAttack, DecalExtent);
	DOREPLIFETIME(AGroundAttack, ConeAngle);
	DOREPLIFETIME(AGroundAttack, InnerRingPercent);
	DOREPLIFETIME(AGroundAttack, Hostility);
	DOREPLIFETIME(AGroundAttack, DetonationTime);
	DOREPLIFETIME(AGroundAttack, IndicatorColor);
	DOREPLIFETIME(AGroundAttack, IndicatorTexture);
	DOREPLIFETIME(AGroundAttack, Intensity);
	DOREPLIFETIME(AGroundAttack, bDestroyOnDetonate);
}

void AGroundAttack::Initialize(const FVector& InExtent, const float InConeAngle, const float InInnerRingPercent,
	const EFaction InHostility, const float InDetonationTime, const bool bInDestroyOnDetonate, const FLinearColor& InIndicatorColor,
	UTexture2D* InIndicatorTexture, const float InIntensity)
{
	SetDecalExtent(InExtent);
	SetConeAngle(InConeAngle);
	SetInnerRingPercent(InInnerRingPercent);
	SetHostility(InHostility);
	SetDetonationTime(InDetonationTime, bInDestroyOnDetonate);
	SetIndicatorColor(InIndicatorColor);
	SetIndicatorTexture(InIndicatorTexture);
	SetIntensity(InIntensity);
}

void AGroundAttack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsValid(GetWorld()->GetGameState()))
	{
		return;
	}
	if (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() >= DetonationTime)
	{
		StartTime = 0.0f;
		SetActorTickEnabled(false);
		DoDetonation();
		if (IsValid(DecalMaterial))
		{
			DecalMaterial->SetScalarParameterValue(FName(TEXT("Percent")), 0.0f);
		}
	}
	else
	{
		if (IsValid(DecalMaterial))
		{
			const float Percent = FMath::Clamp((GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - StartTime) / (DetonationTime - StartTime), 0.0f, 1.0f);
			DecalMaterial->SetScalarParameterValue(FName(TEXT("Percent")), Percent);
		}
	}
}

void AGroundAttack::SetDecalExtent(const FVector NewExtent)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	DecalExtent = FVector(FMath::Max(1.0f, NewExtent.X), FMath::Max(1.0f, NewExtent.Y), FMath::Max(1.0f, NewExtent.Z));
	OnRep_DecalExtent();
}

void AGroundAttack::OnRep_DecalExtent()
{
	SetActorScale3D(FVector(DecalExtent.Z, DecalExtent.Y, DecalExtent.X) / 100.0f);
}

void AGroundAttack::SetConeAngle(const float NewConeAngle)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	ConeAngle = FMath::Clamp(NewConeAngle, 0.0f, 360.0f);
	OnRep_ConeAngle();
}

void AGroundAttack::OnRep_ConeAngle()
{
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetScalarParameterValue(FName(TEXT("ConeAngle")), ConeAngle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Decal material wasn't valid in OnRep_ConeAngle."));
	}
}

void AGroundAttack::SetInnerRingPercent(const float NewInnerRingPercent)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	InnerRingPercent = FMath::Clamp(NewInnerRingPercent, 0.0f, 100.0f);
	OnRep_InnerRingPercent();
}

void AGroundAttack::OnRep_InnerRingPercent()
{
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetScalarParameterValue(FName(TEXT("InnerRadius")), InnerRingPercent);
	}
}

void AGroundAttack::SetHostility(const EFaction NewHostility)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	Hostility = NewHostility;
	OnRep_Hostility();
}

void AGroundAttack::OnRep_Hostility()
{
	if (IsValid(OuterBox))
	{
		switch(Hostility)
		{
		case EFaction::Friendly :
			OuterBox->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxPlayers);
			break;
		case EFaction::Enemy :
			OuterBox->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxNPCs);
			break;
		case EFaction::Neutral :
			OuterBox->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxAll);
			break;
		default :
			OuterBox->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
			break;
		}
	}
}

void AGroundAttack::SetIndicatorColor(const FLinearColor NewIndicatorColor)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	IndicatorColor = NewIndicatorColor;
	OnRep_IndicatorColor();
}

void AGroundAttack::OnRep_IndicatorColor()
{
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetVectorParameterValue(FName(TEXT("Color")), IndicatorColor);
	}
}

void AGroundAttack::SetIndicatorTexture(UTexture2D* NewIndicatorTexture)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	IndicatorTexture = NewIndicatorTexture;
	OnRep_IndicatorTexture();
}

void AGroundAttack::OnRep_IndicatorTexture()
{
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetTextureParameterValue(FName(TEXT("Texture")), IndicatorTexture);
	}
}

void AGroundAttack::SetIntensity(const float NewIntensity)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	Intensity = FMath::Max(0.0f, NewIntensity);
	OnRep_Intensity();
}

void AGroundAttack::OnRep_Intensity()
{
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetScalarParameterValue(FName(TEXT("Intensity")), Intensity);
	}
}

void AGroundAttack::SetDetonationTime(const float NewDetonationTime, const bool bInDestroyOnDetonate)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	DetonationTime = NewDetonationTime;
	bDestroyOnDetonate = bInDestroyOnDetonate;
	OnRep_DetonationTime();
}

void AGroundAttack::OnRep_DetonationTime()
{
	if (!IsValid(GetWorld()->GetGameState()))
	{
		return;
	}
	if (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() < DetonationTime)
	{
		StartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		SetActorTickEnabled(true);
	}
	else
	{
		StartTime = 0.0f;
		SetActorTickEnabled(false);
	}
	if (IsValid(DecalMaterial))
	{
		DecalMaterial->SetScalarParameterValue(FName(TEXT("Percent")), 0.0f);
	}
}

void AGroundAttack::DoDetonation()
{
	TArray<AActor*> HitActors;
	GetActorsInDetonation(HitActors);
	OnDetonation.Broadcast(this, HitActors);
	if (bDestroyOnDetonate)
	{
		Destroy();
	}
}

void AGroundAttack::GetActorsInDetonation(TArray<AActor*>& HitActors) const
{
	HitActors.Empty();
	if (!IsValid(OuterBox) || Hostility == EFaction::None)
	{
		return;
	}
	TArray<AActor*> Overlapping;
	OuterBox->GetOverlappingActors(Overlapping);
	for (AActor* Actor : Overlapping)
	{
		if (!Actor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			continue;
		}
		
		UCombatStatusComponent* Combat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Actor);
		if (!IsValid(Combat))
		{
			continue;
		}
		switch (Combat->GetCurrentFaction())
		{
		case EFaction::None :
			continue;
		case EFaction::Friendly :
			if (Hostility == EFaction::Enemy)
			{
				continue;
			}
			break;
		case EFaction::Enemy :
			if (Hostility == EFaction::Friendly)
			{
				continue;
			}
			break;
		default:
			break;
		}

		const float AngleToActor = GetAngleToActor(Actor);
		const float RadiusAtAngle = GetRadiusAtAngle(AngleToActor);
		const float XYDistance = FVector::DistXY(Actor->GetActorLocation(), GetActorLocation());
		if (XYDistance > RadiusAtAngle)
		{
			continue;
		}
		if (InnerRingPercent > 0.0f)
		{
			if (XYDistance < RadiusAtAngle * (InnerRingPercent / 100.0f))
			{
				continue;
			}
		}
		if (FMath::Abs(AngleToActor) > (ConeAngle / 2))
		{
			continue;
		}
		HitActors.AddUnique(Actor);
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
	const float Top = DecalExtent.X * DecalExtent.Y;
	const float XSquared = FMath::Square(DecalExtent.X * UKismetMathLibrary::DegSin(Angle));
	const float YSquared = FMath::Square(DecalExtent.Y * UKismetMathLibrary::DegCos(Angle));
	return (Top / (UKismetMathLibrary::Sqrt(XSquared + YSquared)));
}