#pragma once
#include "CoreMinimal.h"
#include "CanvasPanel.h"
#include "Image.h"
#include "SizeBox.h"
#include "SizeBoxSlot.h"
#include "UserWidget.h"
#include "Crosshair.generated.h"

class ASaiyoraPlayerCharacter;

USTRUCT()
struct FCrosshairComponent
{
	GENERATED_BODY()

	UPROPERTY()
	UImage* Image;
	UPROPERTY()
	UMaterialInstanceDynamic* Material;
	UPROPERTY()
	USizeBox* SizeBox;
	UPROPERTY()
	USizeBoxSlot* SizeBoxSlot;
	UPROPERTY()
	UCanvasPanelSlot* CanvasSlot;
};

UCLASS()
class SAIYORAV4_API UCrosshair : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* PlayerRef);
	void UpdateSpread(const float Range, const float Angle);
	void UpdateColor(const FColor NewColor);
	
private:

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* CrosshairCanvas;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	int NumOuterImages = 4;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float OffsetAngle = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float OuterImageSize = 64.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UMaterialInstance* OuterImageMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (InlineEditConditionToggle))
	bool bShowCenterRing = false;
	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (EditCondition = "bShowCenterRing"))
	float CenterRingWidth = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UMaterialInstance* CenterRingMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (InlineEditConditionToggle))
	bool bShowCenterDot = false;
	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (EditCondition = "bShowCenterDot"))
	float CenterDotSize = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UMaterialInstance* CenterDotMaterial;
	
	UPROPERTY()
	TArray<FCrosshairComponent> OuterImages;
	UPROPERTY()
	FCrosshairComponent CenterRing;
	UPROPERTY()
	FCrosshairComponent CenterDot;

	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerChar;

	//Helper to get the angle to rotate by and offset in the direction of for the crosshair outer images.
	float GetAngleOffset(const int Index) const;
	//Helper to convert the offset angle to an FVector2D offset in that direction of half the outer image size.
	//This is the offset of each outer image at 0 spread.
	FVector2D GetBaseImageOffset(const float AngleDegrees) const;
};
