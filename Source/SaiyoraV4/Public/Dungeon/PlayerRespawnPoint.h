#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/TargetPoint.h"
#include "PlayerRespawnPoint.generated.h"

class ADungeonGameState;

UCLASS()
class SAIYORAV4_API APlayerRespawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:

	APlayerRespawnPoint();
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (Categories = "Boss"))
	FGameplayTag BossTriggerTag;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;
	bool bActivated = false;

	UFUNCTION()
	void OnBossKilled(const FGameplayTag BossTag, const FString& BossName);
};
