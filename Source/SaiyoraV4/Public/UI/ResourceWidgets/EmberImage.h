#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "EmberImage.generated.h"

class UImage;

UCLASS()
class SAIYORAV4_API UEmberImage : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetActive(const bool bActive);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* EmberImage;
};
