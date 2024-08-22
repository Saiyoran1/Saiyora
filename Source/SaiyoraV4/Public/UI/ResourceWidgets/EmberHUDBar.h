#pragma once
#include "CoreMinimal.h"
#include "ResourceBar.h"
#include "EmberHUDBar.generated.h"

class UEmberImage;
class UImage;
class UHorizontalBox;

UCLASS()
class SAIYORAV4_API UEmberHUDBar : public UResourceBar
{
	GENERATED_BODY()

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHorizontalBox* EmberBox;

	UPROPERTY(EditDefaultsOnly, Category = "Embers")
	TSubclassOf<UEmberImage> EmberWidgetClass;

	UPROPERTY()
	TArray<UEmberImage*> EmberWidgets;
	
	virtual void OnChange(const FResourceState& PreviousState, const FResourceState& NewState) override;
};
