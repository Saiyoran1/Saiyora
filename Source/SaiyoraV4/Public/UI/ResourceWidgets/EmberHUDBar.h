#pragma once
#include "CoreMinimal.h"
#include "ResourceBar.h"
#include "EmberHUDBar.generated.h"

class UOverlay;
class UEmberImage;
class UImage;

UCLASS()
class SAIYORAV4_API UEmberHUDBar : public UResourceBar
{
	GENERATED_BODY()

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UOverlay* RootSlot;
	UPROPERTY(EditAnywhere, Category = "Embers")
	float VerticalExtent = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Embers")
	float HorizontalExtent = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Embers")
	TSubclassOf<UEmberImage> EmberWidgetClass;

	UPROPERTY()
	TArray<UEmberImage*> EmberWidgets;
	
	virtual void OnChange(const FResourceState& PreviousState, const FResourceState& NewState) override;
};
