#include "PredictableProjectile.h"
#include "AbilityComponent.h"
#include "CombatAbility.h"
#include "CombatDebugOptions.h"
#include "CombatNetSubsystem.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraProjectileComponent.h"
#include "UnrealNetwork.h"
#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"

APredictableProjectile::APredictableProjectile(const class FObjectInitializer& ObjectInitializer)
{
	ProjectileMovement = CreateDefaultSubobject<USaiyoraProjectileComponent>(TEXT("Projectile Movement"));
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

void APredictableProjectile::PostNetReceiveLocationAndRotation()
{
    ProjectileMovement->MoveInterpolationTarget(GetActorLocation(), GetActorRotation());
    Super::PostNetReceiveLocationAndRotation();
}

void APredictableProjectile::PostNetInit()
{
	Super::PostNetInit();
	
	if (IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled() && !bShouldReplace)
	{
		const USaiyoraGameInstance* GameInstance = Cast<USaiyoraGameInstance>(GetWorld()->GetGameInstance());
		if (IsValid(GameInstance))
		{
			DebugOptions = GameInstance->CombatDebugOptions;
		}
		
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* Comp : PrimitiveComponents)
		{
			PreHideCollision.Add(Comp, Comp->GetCollisionProfileName());
		}
		HideProjectile();
	}
}

void APredictableProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (bHidden && IsValid(DebugOptions) && DebugOptions->bDrawHiddenProjectiles)
	{
		DebugOptions->DrawHiddenProjectile(this);
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
	SourceInfo.SourceAbility = Source;
	if (Source->GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Source->GetHandler()->OnAbilityMispredicted.AddDynamic(this, &APredictableProjectile::DeleteOnMisprediction);
		UCombatNetSubsystem* NetSubsystem = GetWorld()->GetSubsystem<UCombatNetSubsystem>();
		if (IsValid(NetSubsystem))
		{
			NetSubsystem->RegisterClientProjectile(SourceInfo.Owner, this);
		}
	}
	else if (Source->GetHandler()->GetOwnerRole() == ROLE_Authority)
	{
		SourceInfo.ID = ID;
		//TODO: Currently no lag comp cap here?
		ProjectileMovement->SetInitialCatchUpTime(this, USaiyoraCombatLibrary::GetActorPing(Source->GetHandler()->GetOwner()));
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
	OnInitialize();

	const USaiyoraGameInstance* GameInstance = Cast<USaiyoraGameInstance>(GetWorld()->GetGameInstance());
	if (IsValid(GameInstance))
	{
		DebugOptions = GameInstance->CombatDebugOptions;
	}
}

void APredictableProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(APredictableProjectile, SourceInfo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(APredictableProjectile, bShouldReplace, COND_OwnerOnly);
	DOREPLIFETIME(APredictableProjectile, bDestroyed);
}

void APredictableProjectile::OnRep_SourceInfo()
{
	//If bShouldReplace is already true when we first replicate to this client, go ahead and replace the predicted projectile.
	if (!bReplaced && bShouldReplace && IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled())
	{
		UCombatNetSubsystem* NetSubsystem = GetWorld()->GetSubsystem<UCombatNetSubsystem>();
		if (IsValid(NetSubsystem))
		{
			NetSubsystem->ReplaceProjectile(this);
			bReplaced = true;
			bHidden = false;
		}
	}
}

void APredictableProjectile::OnRep_ShouldReplace()
{
	//When the server replicates that it's time to replace the predicted projectile, do so here.
	//This may have already happened in OnRep_SourceInfo.
	if (!bReplaced && bShouldReplace && IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled())
	{
		UCombatNetSubsystem* NetSubsystem = GetWorld()->GetSubsystem<UCombatNetSubsystem>();
		if (IsValid(NetSubsystem))
		{
			NetSubsystem->ReplaceProjectile(this);
			for (const TTuple<UPrimitiveComponent*, FName>& Collision : PreHideCollision)
			{
				if (IsValid(Collision.Key))
				{
					Collision.Key->SetCollisionProfileName(Collision.Value);
					Collision.Key->SetVisibility(true);
				}
			}
			bReplaced = true;
			bHidden = false;
		}
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
	HideProjectile();
	Destroy();
	return bClientDestroyed;
}

void APredictableProjectile::UpdateLocallyDestroyed(bool const bLocallyDestroyed)
{
	if (bLocallyDestroyed)
	{
		bClientDestroyed = true;
		HideProjectile();
		OnDestroy();
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
			bClientDestroyed = true;
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
	bHidden = true;
}

#pragma endregion 
