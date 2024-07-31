#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "BuffBar.generated.h"

struct FBuffApplyEvent;
class UBuff;
class UPlayerHUD;
class UTextBlock;
class UProgressBar;
class UImage;

UCLASS()
class SAIYORAV4_API UBuffBar : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void Init(UPlayerHUD* OwnerHUD);
	void SetBuff(UBuff* NewBuff);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* DurationBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* IconImage;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* NameText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* DescriptionText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* StackText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* DurationText;

	UPROPERTY()
	UPlayerHUD* OwningHUD = nullptr;
	bool bShowingExtraInfo = false;
	UFUNCTION()
	void ToggleExtraInfo(const bool bShowExtraInfo);

	UPROPERTY()
	UBuff* AssignedBuff = nullptr;
	UFUNCTION()
	void OnBuffUpdated(const FBuffApplyEvent& ApplyEvent);
};
