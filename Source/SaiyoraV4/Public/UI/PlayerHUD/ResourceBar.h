#pragma once
#include "CoreMinimal.h"
#include "ResourceStructs.h"
#include "UserWidget.h"
#include "ResourceBar.generated.h"

class UPlayerHUD;
struct FResourceState;
class UResource;
class UTextBlock;
class UProgressBar;

UCLASS()
class SAIYORAV4_API UResourceBar : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitResourceBar(UResource* Resource);

protected:

	UResource* GetResource() const { return OwnerResource; }

private:

	UPROPERTY()
	UResource* OwnerResource = nullptr;

	UFUNCTION()
	void OnResourceChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState)
	{
		OnChange(PreviousState, NewState);
	}

	virtual void OnInit() {}
	virtual void OnChange(const FResourceState& PreviousState, const FResourceState& NewState) {}
};
