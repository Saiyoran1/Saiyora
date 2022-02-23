#include "PredictableProjectile.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "SaiyoraGameState.h"
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
	GameState = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!IsValid(GameState))
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
		GameState->RegisterClientProjectile(this);
	}
}

void APredictableProjectile::Replace()
{
	//TODO
	UE_LOG(LogTemp, Warning, TEXT("Successfully replacing projectile!"));
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
}

void APredictableProjectile::OnRep_SourceInfo()
{
	if (IsValid(SourceInfo.Owner) && SourceInfo.Owner->IsLocallyControlled() && !bReplaced)
	{
		GameState = GetWorld()->GetGameState<ASaiyoraGameState>();
		if (!IsValid(GameState))
		{
			return;
		}
		GameState->ReplaceProjectile(this);
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
