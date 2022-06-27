#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/CapsuleComponent.h"
#include "PlayerRespawnPoint.generated.h"

class ADungeonGameState;

UCLASS()
class SAIYORAV4_API APlayerRespawnPoint : public AActor
{
	GENERATED_BODY()

public:

	APlayerRespawnPoint();
	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Respawn", meta = (AllowPrivateAccess = true))
	UCapsuleComponent* CapsuleComponent;
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (Categories = "Boss"))
	FGameplayTag BossTriggerTag;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;
	bool bActivated = false;

	UFUNCTION()
	void OnBossKilled(const FGameplayTag BossTag, const FString& BossName);
};
