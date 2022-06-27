#include "PlayerRespawnPoint.h"
#include "DamageHandler.h"
#include "DungeonGameState.h"
#include "SaiyoraPlayerCharacter.h"

APlayerRespawnPoint::APlayerRespawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(FName(TEXT("Capsule")));
	CapsuleComponent->SetCapsuleSize(34.0f, 88.0f);
}

void APlayerRespawnPoint::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_Client)
	{
		GameStateRef = GetWorld()->GetGameState<ADungeonGameState>();
		if (IsValid(GameStateRef))
		{
			GameStateRef->OnBossKilled.AddDynamic(this, &APlayerRespawnPoint::OnBossKilled);
		}
	}
}

void APlayerRespawnPoint::OnBossKilled(const FGameplayTag BossTag, const FString& BossName)
{
	if (!bActivated && BossTag.MatchesTagExact(BossTriggerTag))
	{
		bActivated = true;
		TArray<ASaiyoraPlayerCharacter*> Players;
		GameStateRef->GetActivePlayers(Players);
		for (const ASaiyoraPlayerCharacter* Player : Players)
		{
			ISaiyoraCombatInterface::Execute_GetDamageHandler(Player)->UpdateRespawnPoint(GetActorLocation());
		}
		GameStateRef->OnBossKilled.RemoveDynamic(this, &APlayerRespawnPoint::OnBossKilled);
	}
}
