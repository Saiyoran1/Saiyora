#include "ResourceWidgets/DiscreteResourceBar.h"
#include "Overlay.h"
#include "Resource.h"
#include "SaiyoraUIDataAsset.h"
#include "UIFunctionLibrary.h"
#include "ResourceWidgets/EmberImage.h"

void UDiscreteResourceBar::OnChange(const FResourceState& PreviousState, const FResourceState& NewState)
{
	if (!IsValid(RootSlot))
	{
		return;
	}
	bool bResetTransforms = false;
	//Add more icons if the maximum resource value increased.
	if (NewState.Maximum > IconWidgets.Num())
	{
		if (!IsValid(IconWidgetClass) || !IsValid(RootSlot))
		{
			return;
		}
		while (IconWidgets.Num() < NewState.Maximum)
		{
			UDiscreteResourceIcon* IconWidget = CreateWidget<UDiscreteResourceIcon>(this, IconWidgetClass);
			if (!IsValid(IconWidget))
			{
				return;
			}
			IconWidgets.Add(IconWidget);
			RootSlot->AddChildToOverlay(IconWidget);
			bResetTransforms = true;
		}
	}
	//Remove icon widgets if the maximum resource value decreased.
	else if (IconWidgets.Num() > NewState.Maximum)
	{
		while (IconWidgets.Num() > NewState.Maximum)
		{
			UDiscreteResourceIcon* IconWidget = IconWidgets.Pop();
			if (IsValid(IconWidget))
			{
				IconWidget->RemoveFromParent();
				bResetTransforms = true;
			}
		}
	}
	//If the number of icons changed, we need to reset the icons to new positions.
	if (bResetTransforms && IconWidgets.Num() > 0)
	{
		//Icons are arranged in a vertical arc to the side of the center of the screen.
		//We do this by making a big circle at the location of the resource bar, and placing icons along the edge of the circle via render transform (their actual locations are all stacked inside an overlay).
		//Currently, I'm using only a portion of the circle, and making the circle fairly large, to give the illusion of an ellipse shape.
		//Then, we offset all of the icon positions along the x axis, so that the middle icon is where the circle center was.
		//This just means the center of the arc will always be at the position of this widget.
		const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
		//Get the radius of the circle we'll be placing icons on, and the angle extent (how much of the circle) we'll spread them across.
		const float Radius = IsValid(UIDataAsset) ? UIDataAsset->HUDResourceBarRadius : 240.0f;
		const float MaxAngleExtent = IsValid(UIDataAsset) ? UIDataAsset->HUDResourceBarAngleExtent : 110.0f;
		//If we don't have very many icons, we might use less than the max angle extent, so as not to spread them out too much.
		//We just check if the size of the icons times the number of icons is smaller than the radius.
		//TODO: I don't know if this math is good. We might need to not use Radius here and instead use the actual vertical distance between the top and bottom of the arc.
		const float PercentMaxAngle = FMath::Clamp(Radius / (IconWidgets[0]->GetDesiredSize().X * IconWidgets.Num()), 0.0f, 1.0f);
		const float AngleExtent = MaxAngleExtent * PercentMaxAngle;
		const float AngleBetween = AngleExtent / IconWidgets.Num();
		//Get the "center" index, so that the icons will always center on the horizontal axis of our circle.
		const float Center = (IconWidgets.Num() / 2.0f) - 0.5f;
		for (int i = 0; i < IconWidgets.Num(); i++)
		{
			const float Theta = AngleBetween * (i - Center);
			const float ThetaRad = FMath::DegreesToRadians(Theta);
			//sin(Theta) = Y / Radius;
			const float Y = -1.0f * FMath::Sin(ThetaRad) * Radius;
			//cos(Theta) = X / Radius;
			const float X = (FMath::Cos(ThetaRad) * Radius) - Radius;
			IconWidgets[i]->SetRenderTransform(FWidgetTransform(FVector2D(X, Y), FVector2D(1.0f), FVector2D(0.0f), 0.0f));
		}
	}
	//Activate and deactivate the widgets based on index to represent how much resource is available.
	for (int i = 0; i < IconWidgets.Num(); i++)
	{
		if (IsValid(IconWidgets[i]))
		{
			IconWidgets[i]->SetActive(i < NewState.CurrentValue);
		}
	}
}