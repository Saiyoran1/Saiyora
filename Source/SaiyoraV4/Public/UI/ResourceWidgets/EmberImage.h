#pragma once
#include "CoreMinimal.h"
#include "DiscreteResourceIcon.h"
#include "EmberImage.generated.h"

class UImage;

UCLASS()
class SAIYORAV4_API UEmberImage : public UDiscreteResourceIcon
{
	GENERATED_BODY()

protected:

	virtual void OnSetActive(const bool bNewActive) override;

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* EmberImage;
};
