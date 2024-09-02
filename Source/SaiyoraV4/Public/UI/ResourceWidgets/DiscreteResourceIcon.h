#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "DiscreteResourceIcon.generated.h"

UCLASS()
class SAIYORAV4_API UDiscreteResourceIcon : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetActive(const bool bNewActive)
	{
		if (bActive == bNewActive && bFirstSet)
		{
			return;
		}
		bFirstSet = true;
		bActive = bNewActive;
		OnSetActive(bActive);
	}
	virtual void OnSetActive(const bool bNewActive) {}

private:

	bool bActive = false;
	bool bFirstSet = false;
};
