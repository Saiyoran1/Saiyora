#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "Image.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "BuffIcon.generated.h"

class UBuff;

UCLASS()
class SAIYORAV4_API UBuffIcon : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(UBuff* AssignedBuff);
	void Cleanup();
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UPROPERTY(meta = (BindWidget))
	UImage* BuffIcon;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StackText;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	UMaterialInstance* BuffMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BuffMI;

	UPROPERTY()
	UBuff* Buff;
	UFUNCTION()
	void OnBuffUpdated(const FBuffApplyEvent& Event);
	void UpdateStacks(const int NewStacks);
};
