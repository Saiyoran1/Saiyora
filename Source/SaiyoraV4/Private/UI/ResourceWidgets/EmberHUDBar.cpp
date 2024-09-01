#include "ResourceWidgets/EmberHUDBar.h"
#include "Overlay.h"
#include "Resource.h"
#include "ResourceWidgets/EmberImage.h"

void UEmberHUDBar::OnChange(const FResourceState& PreviousState, const FResourceState& NewState)
{
	if (!IsValid(RootSlot))
	{
		return;
	}
	bool bResetTransforms = false;
	if (NewState.Maximum > EmberWidgets.Num())
	{
		if (!IsValid(EmberWidgetClass) || !IsValid(RootSlot))
		{
			return;
		}
		while (EmberWidgets.Num() < NewState.Maximum)
		{
			UEmberImage* EmberImage = CreateWidget<UEmberImage>(this, EmberWidgetClass);
			if (!IsValid(EmberImage))
			{
				return;
			}
			EmberWidgets.Add(EmberImage);
			RootSlot->AddChildToOverlay(EmberImage);
			bResetTransforms = true;
		}
	}
	else if (EmberWidgets.Num() > NewState.Maximum)
	{
		while (EmberWidgets.Num() > NewState.Maximum)
		{
			UEmberImage* EmberImage = EmberWidgets.Pop();
			if (IsValid(EmberImage))
			{
				EmberImage->RemoveFromParent();
				bResetTransforms = true;
			}
		}
	}
	if (bResetTransforms && EmberWidgets.Num() > 0)
	{
		//TODO: This needs to be equidistant along the arc, NOT equal angles.
		//Radius at angle on an ellipse with horizontal axis a and vertical axis b
		//r = (ab) / sqrt(a^2 * sin^2(theta) + b^2 * cos^2(theta))
		const float AB = VerticalExtent * HorizontalExtent;
		const float PercentMaxAngle = FMath::Clamp(VerticalExtent / (EmberWidgets[0]->GetDesiredSize().X * EmberWidgets.Num()), 0.0f, 1.0f);
		const float AngleExtent = 180.0f * PercentMaxAngle;
		const float AngleBetween = AngleExtent / EmberWidgets.Num();
		const float Center = (EmberWidgets.Num() / 2.0f) - 0.5f;
		for (int i = 0; i < EmberWidgets.Num(); i++)
		{
			const float Theta = AngleBetween * (i - Center);
			const float ThetaRad = FMath::DegreesToRadians(Theta);
			const float Radius = AB / (FMath::Sqrt(
				(FMath::Square(HorizontalExtent) * FMath::Square(FMath::Sin(ThetaRad)))
				+ (FMath::Square(VerticalExtent) * FMath::Square(FMath::Cos(ThetaRad)))));
			//sin(Theta) = Y / Radius;
			const float Y = -1.0f * FMath::Sin(ThetaRad) * Radius;
			//cos(Theta) = X / Radius;
			const float X = FMath::Cos(ThetaRad) * Radius;
			EmberWidgets[i]->SetRenderTransform(FWidgetTransform(FVector2D(X, Y), FVector2D(1.0f), FVector2D(0.0f), 0.0f));
		}
	}
	for (int i = 0; i < EmberWidgets.Num(); i++)
	{
		if (IsValid(EmberWidgets[i]))
		{
			EmberWidgets[i]->SetActive(i < NewState.CurrentValue);
		}
	}
}