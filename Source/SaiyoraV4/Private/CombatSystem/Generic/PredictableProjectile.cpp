#include "PredictableProjectile.h"
#include "AbilityComponent.h"
#include "CombatAbility.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"

APredictableProjectile::APredictableProjectile(const class FObjectInitializer& ObjectInitializer)
{
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

void APredictableProjectile::PostNetReceiveLocationAndRotation()
{
    ProjectileMovement->MoveInterpolationTarget(GetActorLocation(), GetActorRotation());
    Super::PostNetReceiveLocationAndRotation();
}

void APredictableProjectile::Tick(float DeltaSeconds)
{
	if (RemainingLag > 0.0f)
	{
		//Calculate time to add, we want to tick twice as fast until we catch up.
		const float AddTime = FMath::Min(DeltaSeconds, RemainingLag);
		RemainingLag -= AddTime;
		Super::Tick(DeltaSeconds + AddTime);
	}
	else
	{
		Super::Tick(DeltaSeconds);
	}
}

void APredictableProjectile::InitializeProjectile(UCombatAbility* Source, const FPredictedTick& Tick, const int32 ID,
                                                  const ESaiyoraPlane ProjectilePlane, const EFaction ProjectileHostility)
{
	if (!IsValid(Source))
	{
		return;
	}
	bIsFake = GetOwner()->GetLocalRole() != ROLE_Authority;
	SourceInfo.Owner = Cast<ASaiyoraPlayerCharacter>(GetOwner());
	SourceInfo.SourceClass = Source->GetClass();
	SourceInfo.SourceTick = Tick;
	SourceInfo.ID = ID;
	SourceInfo.SourceAbility = Source;
	if (Source->GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Source->GetHandler()->OnAbilityMispredicted.AddDynamic(this, &APredictableProjectile::DeleteOnMisprediction);
		SourceInfo.Owner->RegisterClientProjectile(this);
	}
	else if (Source->GetHandler()->GetOwnerRole() == ROLE_Authority)
	{
		RemainingLag = USaiyoraCombatLibrary::GetActorPing(Source->GetHandler()->GetOwner());
	}
	DestroyDelegate.BindUObject(this, &APredictableProjectile::DelayedDestroy);
	
	TArray<UPrimitiveComponent*> Components;
	GetComponents<UPrimitiveComponent>(Components);
	for (UPrimitiveComponent* Comp : Components)
	{
		if (Comp->GetCollisionObjectType() == FSaiyoraCollision::O_ProjectileHitbox)
		{
			switch (ProjectileHostility)
			{
			case EFaction::Friendly :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxPlayers);
				break;
			case EFaction::Enemy :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxNPCs);
				break;
			case EFaction::Neutral :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileHitboxAll);
				break;
			default :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
				break;
			}
		}
		else if (Comp->GetCollisionObjectType() == FSaiyoraCollision::O_ProjectileCollision)
		{
			switch (ProjectilePlane)
			{
			case ESaiyoraPlane::Ancient :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileCollisionAncient);
				break;
			case ESaiyoraPlane::Modern :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileCollisionModern);
				break;
			case ESaiyoraPlane::Both :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_ProjectileCollisionAll);
				break;
			case ESaiyoraPlane::Neither :
				Comp->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
				break;
			default :
				//TODO: A profile for projectiles that only hit non-plane geometry?
				break;
			}
		}
	}
}

void APredictableProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(APredictableProjectile, SourceInfo, COND_OwnerOnly);
	DOREPLIFETIME(APredictableProjectile, bDestroyed);
}

void APredictableProjectile::OnRep_SourceInfo()
{
	if (IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled() && !bReplaced)
	{
		SourceInfo.Owner->ReplaceProjectile(this);
		bReplaced = true;
	}
}

void APredictableProjectile::DeleteOnMisprediction(int32 const PredictionID)
{
	if (PredictionID == SourceInfo.SourceTick.PredictionID)
	{
		Destroy();
	}
}

#pragma region Destroy

bool APredictableProjectile::Replace()
{
	Destroy();
	return bClientDestroyed;
}

void APredictableProjectile::UpdateLocallyDestroyed(bool const bLocallyDestroyed)
{
	if (bLocallyDestroyed)
	{
		bClientDestroyed = true;
		HideProjectile();
	}
}

void APredictableProjectile::DestroyProjectile()
{
	if (!IsValid(SourceInfo.Owner))
	{
		return;
	}
	if (SourceInfo.Owner->HasAuthority())
	{
		if (!bDestroyed)
		{
			bDestroyed = true;
			GetWorld()->GetTimerManager().SetTimer(DestroyHandle, DestroyDelegate, 2.0f, false);
			HideProjectile();
			OnDestroy();
		}
	}
	else
	{
		if (!bClientDestroyed)
		{
			bClientDestroyed = true;
			HideProjectile();
			OnDestroy();
		}
	}
}

void APredictableProjectile::OnRep_Destroyed()
{
	if (bDestroyed)
	{
		if (!bClientDestroyed)
		{
			OnDestroy();
			HideProjectile();
		}
		Destroy();
	}
}

void APredictableProjectile::DelayedDestroy()
{
	Destroy();
}

void APredictableProjectile::HideProjectile()
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* Comp : PrimitiveComponents)
	{
		Comp->SetVisibility(false, true);
		Comp->SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	}
}

#pragma endregion 
