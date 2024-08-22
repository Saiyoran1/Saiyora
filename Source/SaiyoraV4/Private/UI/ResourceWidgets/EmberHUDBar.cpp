#include "ResourceWidgets/EmberHUDBar.h"
#include "HorizontalBox.h"
#include "Resource.h"
#include "ResourceWidgets/EmberImage.h"

void UEmberHUDBar::OnChange(const FResourceState& PreviousState, const FResourceState& NewState)
{
	if (!IsValid(EmberBox))
	{
		return;
	}
	if (NewState.Maximum > EmberWidgets.Num())
	{
		if (!IsValid(EmberWidgetClass) || !IsValid(EmberBox))
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
			}
		}
	}
	for (int i = 0; i < EmberWidgets.Num(); i++)
	{
		if (i < NewState.CurrentValue)
		{
			EmberWidgets[i]->SetActive(true);
		}
		else
		{
			EmberWidgets[i]->SetActive(false);
		}
	}
}