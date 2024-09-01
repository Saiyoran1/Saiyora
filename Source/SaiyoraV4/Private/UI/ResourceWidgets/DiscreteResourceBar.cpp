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
	if (bResetTransforms && IconWidgets.Num() > 0)
	{
		const USaiyoraUIDataAsset* UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
		const float Radius = IsValid(UIDataAsset) ? UIDataAsset->HUDResourceBarRadius : 240.0f;
		const float MaxAngleExtent = IsValid(UIDataAsset) ? UIDataAsset->HUDResourceBarAngleExtent : 110.0f;
		
		const float PercentMaxAngle = FMath::Clamp(Radius / (IconWidgets[0]->GetDesiredSize().X * IconWidgets.Num()), 0.0f, 1.0f);
		const float AngleExtent = MaxAngleExtent * PercentMaxAngle;
		const float AngleBetween = AngleExtent / IconWidgets.Num();
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
	for (int i = 0; i < IconWidgets.Num(); i++)
	{
		if (IsValid(IconWidgets[i]))
		{
			IconWidgets[i]->OnSetActive(i < NewState.CurrentValue);
		}
	}
}