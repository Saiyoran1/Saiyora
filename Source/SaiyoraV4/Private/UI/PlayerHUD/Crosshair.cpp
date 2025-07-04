#include "Crosshair.h"
#include "CanvasPanelSlot.h"
#include "SaiyoraPlayerCharacter.h"

void UCrosshair::Init(ASaiyoraPlayerCharacter* PlayerRef)
{
	if (!IsValid(PlayerRef))
	{
		return;
	}
	PlayerChar = PlayerRef;
	if (bShowCenterDot && IsValid(CenterDotMaterial))
	{
		CenterDot.Image = NewObject<UImage>(this);
		CenterDot.Material = UMaterialInstanceDynamic::Create(CenterDotMaterial, this);
		CenterDot.Image->SetBrushFromMaterial(CenterDot.Material);
		//CenterDot.Material->SetScalarParameterValue("Size", CenterDotSize);
		//TODO: Set Color
		
		CenterDot.SizeBox = NewObject<USizeBox>(this);
		CenterDot.SizeBoxSlot = Cast<USizeBoxSlot>(CenterDot.SizeBox->AddChild(CenterDot.Image));
		CenterDot.SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
		CenterDot.SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
		CenterDot.SizeBox->SetHeightOverride(CenterDotSize);
		CenterDot.SizeBox->SetWidthOverride(CenterDotSize);
		
		CenterDot.CanvasSlot = CrosshairCanvas->AddChildToCanvas(CenterDot.SizeBox);
		CenterDot.CanvasSlot->SetAlignment(FVector2D(0.5f));
		CenterDot.CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CenterDot.CanvasSlot->SetAutoSize(true);
	}
	if (bShowCenterRing && IsValid(CenterRingMaterial))
	{
		
	}
	if (IsValid(OuterImageMaterial))
	{
		for (int i = 0; i < NumOuterImages; i++)
		{
			FCrosshairComponent& OuterImage = OuterImages.AddDefaulted_GetRef();
			OuterImage.Image = NewObject<UImage>(this);
			OuterImage.Material = UMaterialInstanceDynamic::Create(OuterImageMaterial, this);
			OuterImage.Image->SetBrushFromMaterial(OuterImage.Material);
			
			OuterImage.SizeBox = NewObject<USizeBox>(this);
			OuterImage.SizeBoxSlot = Cast<USizeBoxSlot>(OuterImage.SizeBox->AddChild(OuterImage.Image));
			OuterImage.SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
			OuterImage.SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
			OuterImage.SizeBox->SetHeightOverride(OuterImageSize);
			OuterImage.SizeBox->SetWidthOverride(OuterImageSize);
			OuterImage.SizeBox->SetRenderTransformPivot(FVector2D(0.5f));
			const float AngleDegrees = GetAngleOffset(i);
			OuterImage.SizeBox->SetRenderTransformAngle(AngleDegrees);
			
			OuterImage.CanvasSlot = CrosshairCanvas->AddChildToCanvas(OuterImage.SizeBox);
			OuterImage.CanvasSlot->SetAlignment(FVector2D(0.5f));
			OuterImage.CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			OuterImage.CanvasSlot->SetAutoSize(true);
			OuterImage.CanvasSlot->SetPosition(GetBaseImageOffset(AngleDegrees));
		}
	}
}

float UCrosshair::GetAngleOffset(const int Index) const
{
	return 180.0f + OffsetAngle + (360.0f / NumOuterImages) * Index;
}

FVector2D UCrosshair::GetBaseImageOffset(const float AngleDegrees) const
{
	return FVector2D(-1.0f * FMath::Sin(FMath::DegreesToRadians(AngleDegrees)) * OuterImageSize, FMath::Cos(FMath::DegreesToRadians(AngleDegrees)) * OuterImageSize);
}