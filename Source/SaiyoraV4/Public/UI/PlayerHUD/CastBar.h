#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "CastBar.generated.h"

class USaiyoraUIDataAsset;
struct FAbilityEvent;
struct FCancelEvent;
struct FInterruptEvent;
class UBorder;
struct FCastingState;
class UAbilityComponent;
class UImage;
class UTextBlock;
class UProgressBar;

UCLASS()
class SAIYORAV4_API UCastBar : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void InitCastBar(AActor* OwnerActor, const bool bDisplayDurationText);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* CastProgress;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* CastText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* DurationText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* CastIcon;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UBorder* InterruptibleBorder;

	UPROPERTY()
	UAbilityComponent* AbilityComponentRef = nullptr;

	UFUNCTION()
	void OnCastStateChanged(const FCastingState& PreviousState, const FCastingState& NewState);
	UFUNCTION()
	void OnCastInterrupted(const FInterruptEvent& Event);
	UFUNCTION()
	void OnCastCancelled(const FCancelEvent& Event);
	UFUNCTION()
	void OnCastTick(const FAbilityEvent& Event);
	bool bDisplayDuration = false;

	UPROPERTY()
	USaiyoraUIDataAsset* UIDataAsset = nullptr;

	void StartFade(const float Duration);
	FTimerHandle FadeTimerHandle;
	UFUNCTION()
	void OnFadeTimeFinished();
};
