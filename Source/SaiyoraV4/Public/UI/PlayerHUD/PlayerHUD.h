#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PlayerHUD.generated.h"

class UHealthBar;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInfoToggleNotification, const bool, bMoreInfo);

UCLASS()
class SAIYORAV4_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;

	void ToggleExtraInfo(const bool bShowExtraInfo);
	bool IsExtraInfoToggled() const { return bDisplayingExtraInfo; }
	FInfoToggleNotification OnExtraInfoToggled;

private:

	bool bDisplayingExtraInfo = false;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHealthBar* HealthBar;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
};
