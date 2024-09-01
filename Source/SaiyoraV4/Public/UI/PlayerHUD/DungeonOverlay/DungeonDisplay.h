#pragma once
#include "CoreMinimal.h"
#include "DungeonGameState.h"
#include "UserWidget.h"
#include "DungeonDisplay.generated.h"

class USaiyoraUIDataAsset;
class UImage;
class UBossRequirementDisplay;
class ADungeonGameState;
class UPlayerHUD;
class UVerticalBox;
class UProgressBar;
class UTextBlock;

UCLASS()
class SAIYORAV4_API UDungeonDisplay : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void InitDungeonDisplay(UPlayerHUD* OwningHUD);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* NameText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* TimeText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* TimeProgress;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* DeathCountImage;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* DeathCountText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* KillCountText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* KillCountProgress;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UVerticalBox* BossBox;

	UPROPERTY(EditAnywhere, Category = "Dungeon")
	TSubclassOf<UBossRequirementDisplay> BossReqWidgetClass;
	
	UPROPERTY()
	TMap<FGameplayTag, UBossRequirementDisplay*> BossReqWidgets;

	UPROPERTY()
	UPlayerHUD* OwnerHUD = nullptr;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;
	UPROPERTY()
	const USaiyoraUIDataAsset* UIDataAsset = nullptr;

	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase);
	UFUNCTION()
	void OnDeathCountChanged(const int32 DeathCount, const float PenaltyTime);
	UFUNCTION()
	void OnKillCountChanged(const int32 KillCount, const int32 MaxCount);
	UFUNCTION()
	void OnBossKilled(const FGameplayTag BossTag, const FString& BossName);
	UFUNCTION()
	void OnDungeonDepleted();
};
