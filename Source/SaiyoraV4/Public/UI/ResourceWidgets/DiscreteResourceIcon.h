#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "DiscreteResourceIcon.generated.h"

UCLASS()
class SAIYORAV4_API UDiscreteResourceIcon : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void OnSetActive(const bool bActive) {}
};
