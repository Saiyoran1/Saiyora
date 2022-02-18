#include "PredictableProjectile.h"
#include "AbilityComponent.h"
#include "UnrealNetwork.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

int32 APredictableProjectile::ProjectileID = 0;
FPredictedTick APredictableProjectile::PredictionScope = FPredictedTick();

APredictableProjectile::APredictableProjectile(const class FObjectInitializer& ObjectInitializer)
{
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

void APredictableProjectile::InitializeProjectile(UCombatAbility* Source)
{
	if (!IsValid(Source))
	{
		return;
	}
	bIsFake = Source->GetHandler()->GetOwnerRole() == ROLE_Authority ? false : true;
	SourceInfo.Owner = Cast<ASaiyoraPlayerCharacter>(GetOwner());
	SourceInfo.SourceClass = Source->GetClass();
	SourceInfo.SourceTick = FPredictedTick(Source->GetPredictionID(), Source->GetCurrentTick());
	SourceInfo.ID = GenerateProjectileID(SourceInfo.SourceTick);
	if (Source->GetHandler()->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		OnMisprediction.BindDynamic(this, &APredictableProjectile::DeleteOnMisprediction);
		Source->GetHandler()->SubscribeToAbilityMispredicted(OnMisprediction);
	}
}

void APredictableProjectile::Replace()
{
	//TODO
}

int32 APredictableProjectile::GenerateProjectileID(FPredictedTick const& Scope)
{
	if (!(Scope == PredictionScope))
	{
		ProjectileID = 0;
		PredictionScope = Scope;
	}
	ProjectileID++;
	if (ProjectileID == 0)
	{
		ProjectileID++;
	}
	return ProjectileID;
}

void APredictableProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(APredictableProjectile, SourceInfo, COND_OwnerOnly);
	DOREPLIFETIME(APredictableProjectile, FinalHit);
}

void APredictableProjectile::OnRep_SourceInfo()
{
	if (IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled())
	{
		
	}
}

void APredictableProjectile::OnRep_FinalHit()
{
	UE_LOG(LogTemp, Warning, TEXT("Final Hit was received."));
}

void APredictableProjectile::DeleteOnMisprediction(int32 const PredictionID)
{
	if (PredictionID == SourceInfo.SourceTick.PredictionID)
	{
		Destroy();
	}
}
