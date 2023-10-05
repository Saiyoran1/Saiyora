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
	UPROPERTY()
	FName TargetSocket = NAME_None;
	UPROPERTY()
	FVector2D DesiredPosition = FVector2d::Zero();
	UPROPERTY()
	FVector2D FinalOffset = FVector2d::Zero();
	UPROPERTY()
	FVector2D PreviousOffset = FVector2d::Zero();
	FHealthBarGridSlot DesiredSlot;
	float DistanceFromGridSlot = 0.0f;
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
	
	TMap<FVector2D, FFloatingHealthBarInfo> CentralGridSlots;

	static FVector2D GetGridSlotLocation(const FVector2D CenterScreen, const float WidgetSize, const FHealthBarGridSlot& GridSlot);
	static EGridSlotOffset BoolsToGridSlot(const bool bRight, const bool bTop, const bool bStartHorizontal);
	static void IncrementGridSlot(const bool bClockwise, EGridSlotOffset& GridSlotOffset, FHealthBarGridSlot& GridSlot);
};
