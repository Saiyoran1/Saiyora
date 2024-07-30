#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PlayerHUD.generated.h"

class UHealthBar;
class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeOnInitialized() override;

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHealthBar* HealthBar;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
};
