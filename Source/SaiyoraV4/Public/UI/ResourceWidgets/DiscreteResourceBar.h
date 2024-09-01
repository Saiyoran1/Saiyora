#pragma once
#include "CoreMinimal.h"
#include "ResourceBar.h"
#include "DiscreteResourceBar.generated.h"

class UDiscreteResourceIcon;
class UOverlay;
class UImage;

UCLASS()
class SAIYORAV4_API UDiscreteResourceBar : public UResourceBar
{
	GENERATED_BODY()

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UOverlay* RootSlot;

	UPROPERTY(EditDefaultsOnly, Category = "Embers")
	TSubclassOf<UDiscreteResourceIcon> IconWidgetClass;

	UPROPERTY()
	TArray<UDiscreteResourceIcon*> IconWidgets;
	
	virtual void OnChange(const FResourceState& PreviousState, const FResourceState& NewState) override;
};
