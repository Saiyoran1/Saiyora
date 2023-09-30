#pragma once
#include "CoreMinimal.h"
#include "CanvasPanel.h"
#include "FloatingHealthBar.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBarManager.generated.h"

class ASaiyoraPlayerCharacter;
class UThreatHandler;

USTRUCT()
struct FFloatingHealthBarInfo
{
	GENERATED_BODY()

	UPROPERTY()
	UFloatingHealthBar* WidgetRef = nullptr;
	UPROPERTY()
	AActor* Target = nullptr;
	UPROPERTY()
	USceneComponent* TargetComponent = nullptr;
	UPROPERTY()
	FName TargetSocket = NAME_None;
	UPROPERTY()
	FVector2D DesiredPosition = FVector2d::Zero();
	UPROPERTY()
	bool bOnScreen = false;
};

UCLASS()
class SAIYORAV4_API UFloatingHealthBarManager : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void NewHealthBar(AActor* Target);
	void UpdateHealthBarPositions();

	UPROPERTY()
	UCanvasPanel* CanvasPanelRef;
	UPROPERTY()
	ASaiyoraPlayerCharacter* LocalPlayer;
	UPROPERTY()
	TArray<FFloatingHealthBarInfo> FloatingBars;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFloatingHealthBar> HealthBarClass;
	
	UFUNCTION()
	void OnEnemyCombatChanged(AActor* Combatant, const bool bNewCombat);
};
