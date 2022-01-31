#include "PredictableProjectile.h"
#include "AbilityComponent.h"
#include "UnrealNetwork.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

int32 APredictableProjectile::ProjectileID = 0;

APredictableProjectile::APredictableProjectile(const class FObjectInitializer& ObjectInitializer)
{
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

int32 APredictableProjectile::InitializeClient(UCombatAbility* Source)
{
	if (!IsValid(Source))
	{
		return 0;
	}
	bIsFake = true;
	SourceInfo.Owner = Cast<ASaiyoraPlayerCharacter>(GetOwner());
	SourceInfo.SourceClass = Source->GetClass();
	SourceInfo.SourceTick = FPredictedTick(Source->GetPredictionID(), Source->GetCurrentTick());
	SourceInfo.ID = GenerateProjectileID();
	OnMisprediction.BindDynamic(this, &APredictableProjectile::DeleteOnMisprediction);
	Source->GetHandler()->SubscribeToAbilityMispredicted(OnMisprediction);
	return SourceInfo.ID;
}

void APredictableProjectile::InitializeServer(UCombatAbility* Source, int32 const ClientID, float const PingComp)
{
	if (!IsValid(Source) || ClientID == 0)
	{
		return;
	}
	bIsFake = false;
	SourceInfo.Owner = Cast<ASaiyoraPlayerCharacter>(GetOwner());
	SourceInfo.SourceClass = Source->GetClass();
	SourceInfo.SourceTick = FPredictedTick(Source->GetPredictionID(), Source->GetCurrentTick());
	SourceInfo.ID = ClientID;
	//TODO: Tick actor forward by delta time.
}

int32 APredictableProjectile::GenerateProjectileID()
{
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
