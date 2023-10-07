#pragma once
#include "CoreMinimal.h"
#include "CanvasPanel.h"
#include "FloatingHealthBar.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBarManager.generated.h"

class ASaiyoraPlayerCharacter;
class UThreatHandler;
class UCombatStatusComponent;

UENUM()
enum class EGridSlotOffset : uint8
{
	Right = 0,
	TopRight = 1,
	Top = 2,
	TopLeft = 3,
	Left = 4,
	BottomLeft = 5,
	Bottom = 6,
	BottomRight = 7
};

USTRUCT()
struct FHealthBarGridSlot
{
	GENERATED_BODY()

	int32 X = 0;
	int32 Y = 0;

	bool operator==(const FHealthBarGridSlot& Other) const { return Other.X == X && Other.Y == Y; }
	FHealthBarGridSlot() {}
	FHealthBarGridSlot(const int32 InX, const int32 InY) : X(InX), Y(InY) {}
};

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
	FName TargetSocket = NAME_None;

	FVector2D PreviousOffset = FVector2d::Zero();
	FHealthBarGridSlot PreviousSlot;

	bool bOnScreen = false;
	FVector2D RootPosition = FVector2d::Zero();
	FVector2D FinalOffset = FVector2d::Zero();
	bool bInCenterGrid = false;
	FHealthBarGridSlot DesiredSlot;
};

UCLASS()
class SAIYORAV4_API UFloatingHealthBarManager : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void NewHealthBar(AActor* Target);
	void UpdateHealthBarPositions(const float DeltaTime);

	UPROPERTY()
	UCanvasPanel* CanvasPanelRef;
	UPROPERTY()
	ASaiyoraPlayerCharacter* LocalPlayer;
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerCombatStatus;
	UPROPERTY()
	TArray<FFloatingHealthBarInfo> FloatingBars;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFloatingHealthBar> HealthBarClass;
	
	UFUNCTION()
	void OnEnemyCombatChanged(AActor* Combatant, const bool bNewCombat);
	
	FVector2D CenterScreen = FVector2D(0.0f);
	int32 ViewportX = 0.0f;
	int32 ViewportY = 0.0f;
	static constexpr float ClampPercent = 0.9f;

	static constexpr float GridSlotSize = 60.0f;
	static constexpr int32 GridRadius = 5;

	FVector2D GetGridSlotLocation(const FHealthBarGridSlot& GridSlot) const;
	static EGridSlotOffset BoolsToGridSlot(const bool bRight, const bool bTop, const bool bStartHorizontal);
	static void IncrementGridSlot(const bool bClockwise, EGridSlotOffset& GridSlotOffset, FHealthBarGridSlot& GridSlot);

	FHealthBarGridSlot FindDesiredGridSlot(const FVector2D& RootPosition) const;
	void UpdateHealthBarRootPosition(FFloatingHealthBarInfo& HealthBar) const;
	void ClampBarRootPosition(FVector2D& Root) const;
	bool IsInCenterGrid(const FVector2D& Location) const;
};
